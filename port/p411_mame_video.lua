-- Isolated VDP2 device fixture, not a game-route capture.
local out=assert(os.getenv('P411_OUT'))
local s=manager.machine.devices[':maincpu'].spaces.program
local function reg(o,v) s:write_u16(0x05f80000+o,v) end
local f=0
p411_lines=emu.add_machine_frame_notifier(function()
 f=f+1
 if f==600 then
  -- Park both CPUs in a test loop so only the device fixture supplies video data.
  s:write_u16(0x060ff000,0xaffe);s:write_u16(0x060ff002,0x0009)
  for _,tag in ipairs({':maincpu',':slave'}) do
   local cpu=manager.machine.devices[tag]
   if cpu then cpu.state['SR'].value=0xf0;cpu.state['PC'].value=0x260ff000 end
  end
  for o=0,0x1fe,2 do reg(o,0) end
  reg(0,0x8011);reg(0xe,0x1000);reg(0x20,0x0101);reg(0x28,0x12);reg(0xf8,1)
  reg(0x70,5);reg(0x74,3);reg(0x78,1);reg(0x7c,1)
  reg(0xac,3);reg(0xae,0)
  for o=0x10,0x1e,2 do reg(o,0x4444) end
  for y=0,255 do for x=0,511 do s:write_u8(0x05e00000+y*512+x,((x//8+y//8)%31)+1) end end
  for i=0,31 do s:write_u16(0x05f00000+i*2,i|(i<<5)|(i<<10)) end
  s:write_u16(0x05e60000,0)
  reg(0x9a,0x0e);reg(0xa0,2);reg(0xa2,0)
  for y=0,239 do
   s:write_u32(0x05e40000+y*12,((y%3)*8)<<16)
   s:write_u32(0x05e40004+y*12,((y*8)%248)<<16)
   s:write_u32(0x05e40008+y*12,y%2==0 and 0x10000 or 0x8000)
  end
 end
 if f==610 or f==620 or f==630 or f==640 or f==650 or f==670 or f==700 then
  manager.machine.screens[':screen']:snapshot(out..'/shot-'..f..'.png')
  local fp=assert(io.open(out..'/vdp2-'..f..'.bin','wb'))
  fp:write(s:read_range(0x05e00000,0x05e7ffff,8),s:read_range(0x05f00000,0x05f00fff,8),s:read_range(0x05f80000,0x05f801ff,8));fp:close()
 end
 if f==611 then reg(0x9a,0x1e) end -- same table, one record per two output lines
 if f==621 then
  reg(0x20,0);reg(0xac,0x8003)
  for y=0,239 do local i=y%32;s:write_u16(0x05e60000+y*2,i|(i<<5)|(i<<10)) end
 end
 if f==631 then
  reg(0x20,0x0101);reg(0x28,0x32);reg(0x9a,0);reg(0x70,0);reg(0x74,0)
  for y=0,255 do for x=0,511 do local i=(x//8)%32;s:write_u16(0x05e00000+(y*512+x)*2,0x8000|i|(i<<5)|(i<<10)) end end
 end
 if f==641 then reg(0xec,1) end
 if f==661 then
  reg(0,0x8013);reg(0x20,0);reg(0xec,0);reg(0xac,3);reg(0xe0,0xf);reg(0xf0,5)
  s:write_u16(0x05e60000,0)
  local palette={0,0x1f,0x3e0,0x7c00,0x7fff}
  for i,v in ipairs(palette) do s:write_u16(0x05f00000+(i-1)*2,v) end
  local function cmd(i,ctrl,pmod,col,src,size,xy)
   local a=0x05c00000+i*32
   for o=0,30,2 do s:write_u16(a+o,0) end
   for o,v in pairs({[0]=ctrl,[4]=pmod,[6]=col,[8]=src,[10]=size}) do s:write_u16(a+o,v) end
   for j,v in ipairs(xy or {}) do s:write_u16(a+10+j*2,v&0xffff) end
  end
  cmd(0,9,0,0,0,0,{0,0,0,0,703,239})
  cmd(1,10,0,0,0,0,{0,0})
  cmd(2,4,0x40,0,0,0,{0,0,703,0,703,239,0,239})
  cmd(3,4,0x40,1,0,0,{20,20,180,31,165,100,37,75})
  cmd(4,4,0x40,2,0,0,{52,90,217,84,240,170,44,186})
  cmd(5,2,0xe0,0,0x2000,0x210,{80,140,300,111,270,214,120,219})
  cmd(6,6,0x40,4,0,0,{10,200,340,185})
  cmd(7,0x8000,0,0,0,0,{})
  for y=0,15 do for x=0,15 do s:write_u8(0x05c10000+y*16+x,1+(x//4+y//4)%4) end end
  s:write_u16(0x05d00000,1);s:write_u16(0x05d00002,3);s:write_u16(0x05d00004,1)
 end
 if f==670 then
  local fp=assert(io.open(out..'/commands.bin','wb'))
  fp:write(s:read_range(0x05c00000,0x05c7ffff,8),s:read_range(0x05c80000,0x05cbffff,8),s:read_range(0x05d00000,0x05d0001f,8),s:read_range(0x05e00000,0x05e7ffff,8),s:read_range(0x05f00000,0x05f00fff,8),s:read_range(0x05f80000,0x05f801ff,8));fp:close()
 end
 if f==671 then
  local path=os.getenv('P411_VDP2_SOURCE')
  if not path then manager.machine:exit() else
   local fp=assert(io.open(path,'rb'));local b=fp:read('*a');fp:close();assert(#b==0x81200)
   for i=0,0x7ffff do s:write_u8(0x05e00000+i,b:byte(i+1)) end
   for i=0,0xfff do s:write_u8(0x05f00000+i,b:byte(0x80001+i)) end
   for i=0,0x1fe,2 do reg(i,(b:byte(0x81001+i)<<8)|b:byte(0x81002+i)) end
   reg(0x20,s:read_u16(0x05f80020)&0x30)
   for i=0xf0,0xf6,2 do reg(i,0) end
  end
 end
 if f==701 then manager.machine:exit() end
end)
