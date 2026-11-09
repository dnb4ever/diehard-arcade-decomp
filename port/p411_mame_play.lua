-- P4.11 physical-panel-only reference player; no guest memory writes.
local f=0
local every=tonumber(os.getenv("SNAP_EVERY") or "300")
local out=assert(os.getenv("P411_OUT"))
local log=assert(io.open(out.."/play.log","w"))
local screen=manager.machine.screens[":screen"]
local ending=false
local ports=manager.machine.ioport.ports
local function field(p,n) for k,v in pairs(ports[p].fields) do if k==n then return v end end end
local F={coin=field(":PORTC","Coin 1"),start=field(":PORTC","1 Player Start"),
 up=field(":PORTA","P1 Up"),down=field(":PORTA","P1 Down"),left=field(":PORTA","P1 Left"),right=field(":PORTA","P1 Right"),
 a=field(":PORTA","P1 Button 1"),b=field(":PORTA","P1 Button 2")}
local last=-1; local lastcoin=-10000
sub = emu.add_machine_frame_notifier(function()
  f=f+1
  local s=manager.machine.devices[":maincpu"].spaces["program"]
  local press={}
  local st=s:read_u8(0x06098674)
  if (st==0 or st==3) and f-lastcoin>600 and f>600 then lastcoin=f end
  if f-lastcoin<6 then press.coin=1 elseif f-lastcoin>=30 and f-lastcoin<36 then press.start=1 end
  local task=s:read_u32(0x060995B4); local me= task~=0 and s:read_u32(task+8) or 0

  local ph=f%24
  if me~=0 then
    local mx=s:read_i32(me+8); local mz=s:read_i32(me+0x10)
    local best=nil; local bx,bz=0,0
    for slot=3,6 do
      local t=s:read_u32(0x060995B0+slot*4); local e= t~=0 and s:read_u32(t+8) or 0
      if e~=0 and s:read_u16(e+0x56)~=0 then
        local dx=s:read_i32(e+8)-mx; local dz=s:read_i32(e+0x10)-mz
        local d=math.abs(dx)+2*math.abs(dz)
        if best==nil or d<best then best=d; bx=dx; bz=dz end
      end
    end
    -- These are physical MAME labels, measured against BIOS raw words.
    -- Left/Down reduce world X/Z; the older reference player drove away.
    local px=4096
    if best==nil then if ph<2 then press.a=1 elseif ph>=12 and ph<14 then press.b=1 end
    else
      local tx= bx<0 and "left" or "right"; local tz= bz<0 and "down" or "up"
      local farx=math.abs(bx)>34*px; local nearx=math.abs(bx)<10*px; local offz=math.abs(bz)>6*px
      if farx or offz or nearx then
        if farx then press[tx]=1 elseif nearx then press[tx=="right" and "left" or "right"]=1 end
        if offz then press[tz]=1 end
      elseif ph<2 or (ph>=12 and ph<14) then press[tx]=1
      elseif ph>=4 and ph<6 then press.a=1 elseif ph>=16 and ph<18 then press.b=1 end
    end
  elseif ph<2 then press.a=1 end
  for k,fl in pairs(F) do fl:set_value(press[k] or 0) end
  local sc=s:read_u32(0x06098668)
  if sc~=last then log:write(string.format("F %d scene=%d st=%d\n",f,sc,st)); log:flush(); last=sc end
  if f%300==0 then
    log:write(string.format("STATE F %d scene=%d st=%d task=%08X me=%08X hp=%d",f,sc,st,task,me,me~=0 and s:read_u16(me+0x56) or 0))
    for slot=1,6 do
      local t=s:read_u32(0x060995B0+slot*4); local e=t~=0 and s:read_u32(t+8) or 0
      if e~=0 then log:write(string.format(" [%d %08X x=%d z=%d hp=%d]",slot,e,s:read_i32(e+8)//4096,s:read_i32(e+0x10)//4096,s:read_u16(e+0x56))) end
    end
    local names={};for k,v in pairs(press) do names[#names+1]=k end
    log:write(string.format(" RAW=%04X INPUT %s\n",s:read_u16(0x06000708),table.concat(names,",")));log:flush()
  end
  if f%every==0 or (sc>=73 and sc<=81) then
    screen:snapshot(out.."/shot-"..f..".png")
    log:write(string.format("SNAP F %d scene=%d\n",f,sc)); log:flush()
  end
  if sc>=73 and sc<=81 then
    ending=true
    local fp=assert(io.open(out.."/vdp2-"..f..".bin","wb"))
    fp:write(s:read_range(0x05e00000,0x05e7ffff,8),s:read_range(0x05f00000,0x05f00fff,8),s:read_range(0x05f80000,0x05f801ff,8)); fp:close()
  end
  if f>=300000 or (ending and sc<73) or io.open(out.."/stop","r") then log:close(); manager.machine:exit() end
end)
