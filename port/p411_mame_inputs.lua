-- Physical switch -> real US BIOS raw-word inventory. No guest writes.
local out=assert(os.getenv("P411_OUT"))
local log=assert(io.open(out.."/inputs.log","w"))
local space=manager.machine.devices[":maincpu"].spaces.program
local port=manager.machine.ioport.ports[":PORTA"]
local names={"Up","Down","Left","Right","Button 1","Button 2","Button 3"}
local controls={}
for _,name in ipairs(names) do controls[#controls+1]=assert(port.fields["P1 "..name]) end
local f=0
p411_inputs=emu.add_machine_frame_notifier(function()
  f=f+1
  if f>=600 and f<880 then
    local index=(f-600)//40+1
    if (f-600)%40>=10 and (f-600)%40<20 then
      log:write(string.format("F %d %s raw=%04X port=%02X\n",f,names[index],space:read_u16(0x06000708),space:read_u8(0x00400001)))
      log:flush()
    end
    for i,field in ipairs(controls) do field:set_value(i==index and 1 or 0) end
  end
  if f==900 then log:close(); manager.machine:exit() end
end)
