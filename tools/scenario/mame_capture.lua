-- Scenario checker v1.01. Read-only game probe; only ioport fields are driven.
-- Binary transport: LE u32 JSON length, JSON, low RAM, high RAM, BGRX pixels.
local out = assert(io.open(assert(os.getenv("SCENARIO_PIPE")), "wb"))
local frames = tonumber(assert(os.getenv("SCENARIO_FRAMES")))
local offset = tonumber(os.getenv("SCENARIO_OFFSET") or "0")
local memory = os.getenv("SCENARIO_MEMORY") ~= "0"
local video = os.getenv("SCENARIO_VIDEO") ~= "0"
local input = {}
for line in io.lines(assert(os.getenv("SCENARIO_INPUT"))) do
    local f, p1, p2, sys = line:match("^(%d+)%s+(%x+)%s+(%x+)%s+(%x+)$")
    if f then input[tonumber(f)] = {tonumber(p1,16),tonumber(p2,16),tonumber(sys,16)} end
end
local ports = manager.machine.ioport.ports
local function field(port, name)
    for n, v in pairs(assert(ports[port],port).fields) do
        if n == name then return v end
    end
    error("Missing MAME field "..port.."/"..name)
end
local controls = {}
for player=1,2 do
    local port = player==1 and ":PORTA" or ":PORTB"
    for _, pair in ipairs({{1,"Button 1"},{2,"Button 2"},{4,"Button 3"},
                          {0x40,"Up"},{0x80,"Down"},{0x100,"Left"},{0x200,"Right"}}) do
        controls[#controls+1] = {field(port,"P"..player.." "..pair[2]),player,pair[1]}
    end
end
for _, pair in ipairs({{1,"Coin 1"},{2,"Coin 2"},{4,"Service Mode"},{8,"Service 1"},
                       {16,"1 Player Start"},{32,"2 Players Start"}}) do
    controls[#controls+1] = {field(":PORTC",pair[2]),3,pair[1]}
end
local function drive(f)
    local row = input[f-offset] or {0,0,0}
    for _, c in ipairs(controls) do c[1]:set_value((row[c[2]] & c[3])~=0 and 1 or 0) end
end
local space = manager.machine.devices[":maincpu"].spaces.program
local screen = manager.machine.screens[":screen"]
local function pointer(p) return p>=0x06000000 and p<=0x060ffe00 and (p&3)==0 end
local f = 0
drive(1)
scenario_subscription = emu.add_machine_frame_notifier(function()
    f = f+1
    local actors = {}
    for slot=1,6 do
        local task = space:read_u32(0x060995b0+slot*4)
        local entity = pointer(task) and space:read_u32(task+8) or 0
        local valid = pointer(entity)
        actors[#actors+1] = string.format('{"slot":%d,"active":%s,"id":%d,"hp":%d,"x":%d,"y":%d,"z":%d}',
            slot,valid and "true" or "false",valid and space:read_u8(entity+0x34) or 0,
            valid and space:read_u16(entity+0x56) or 0, valid and space:read_u32(entity+8) or 0,
            valid and space:read_u32(entity+12) or 0, valid and space:read_u32(entity+16) or 0)
    end
    local pixels,w,h = "",0,0
    if video then pixels,w,h = screen:pixels() end
    local json = string.format('{"version":"1.01","frame":%d,"scene":%d,"mode":%d,"arm":%d,"progress":%d,"join1":%d,"join2":%d,"cw":%d,"rng":%d,"counter":%d,"actors":[%s],"ram_bytes":%d,"pixel_bytes":%d,"width":%d,"height":%d}',
        f,space:read_u32(0x06098668),space:read_u16(0x0609979a),space:read_u32(0x06098670),
        space:read_u32(0x06098830),space:read_u8(0x06098674),space:read_u8(0x06098675),
        space:read_u16(0x06098798),space:read_u32(0x0609957c),space:read_u32(0x060987bc),
        table.concat(actors,","),memory and 0x200000 or 0,#pixels,w,h)
    out:write(string.pack("<I4",#json),json)
    if memory then
        out:write(space:read_range(0x00200000,0x002fffff,8))
        out:write(space:read_range(0x06000000,0x060fffff,8))
    end
    out:write(pixels)
    out:flush()
    drive(f+1) -- Set the panel for the NEXT emulated frame, after sampling this one.
    if f>=frames then out:close(); manager.machine:exit() end
end)
