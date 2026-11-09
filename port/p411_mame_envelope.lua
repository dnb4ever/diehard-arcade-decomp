-- Isolated SCSP device fixture, deliberately replaces sound-CPU execution.
-- This is not a game-route capture. Main game memory is not modified.
local out=assert(os.getenv('P411_OUT'))
local log=assert(io.open(out..'/envelope.log','w'))
local space=manager.machine.devices[':maincpu'].spaces.program
local cpu=manager.machine.devices[':audiocpu']
local cases={{15,0,-3,0},{12,2,1,0},{15,15,7,0},{15,0,0,512},{20,14,7,0}}
local function w(o,v) space:write_u16(0x05b00000+o,v) end
local f=0
p411_eg=emu.add_machine_frame_notifier(function()
 f=f+1
 if f==600 then
  cpu.spaces.program:write_u16(0x70000,0x4e72)
  cpu.spaces.program:write_u16(0x70002,0x2700)
  cpu.state['SR'].value=0x2700;cpu.state['PC'].value=0x70000
  for i=0,31 do w(i*32+0xA,31);w(i*32,0) end
  w(0,0x1000)
 end
 for i,c in ipairs(cases) do
  local start=610+(i-1)*20
  if f==start then
   w(0x06,0xffff);w(0x08,31);w(0x0A,(c[2]<<10)|c[1]);w(0x10,((c[3]&15)<<11)|c[4]);w(0,0x1820)
  elseif f==start+1 then w(0,0x1020)
  elseif f==start+7 then
   w(0x408,0)
   log:write(string.format('CASE %d rr=%d krs=%d oct=%d fns=%d monitor=%d raw=%04X\n',i,c[1],c[2],c[3],c[4],space:read_u16(0x05b00408)&31,space:read_u16(0x05b00408)))
   log:flush()
  end
 end
 if f==720 then log:close();manager.machine:exit() end
end)
