#!/usr/bin/env python3
"""Small synthetic oracle tests for alignment, masking and first-divergence reporting."""
import json
from pathlib import Path
import tempfile
import unittest

import check_scenarios as c


def row(f, scene=1):
    return c.normalize_record(dict(frame=f, scene=scene, mode=0, arm=0, progress=0,
        join1=0, join2=0, cw=0, rng=7, counter=f,
        actors=[dict(slot=i, active=False, id=0, hp=0, x=0, y=0, z=0) for i in range(1,7)]))


class CheckerTest(unittest.TestCase):
    def scenario(self, content):
        with tempfile.NamedTemporaryFile(mode='w', dir='work/scenario_v100', suffix='.txt') as f:
            f.write(content)
            f.flush()
            return c.read_scenario(f.name)

    def test_masks_are_held_released_and_overlapped(self):
        s = self.scenario('frames 6\n2 4 200 0 0\n3 3 1 2 10\n')
        self.assertEqual(s.rows[3], (0x201,2,0x10))
        self.assertEqual(s.rows[5], (0,0,0))
        self.assertEqual(c.port_input(s), '2:P1RIGHT:3,3:P1A:1,3:P2B:1,3:START1:1')

    def test_empty_input_does_not_activate_port_default_coin_start(self):
        self.assertEqual(c.port_input(self.scenario('frames 6\n')), '1006:COIN:1')

    def test_unknown_mask_and_out_of_bounds_rejected(self):
        for text in ('frames 6\n1 2 8 0 0\n', 'frames 6\n1 7 1 0 0\n'):
            with self.assertRaises(ValueError):
                self.scenario(text)

    def test_test_switch_fails_loudly_without_hook(self):
        s = self.scenario('frames 6\n1 2 0 0 4\n')
        with self.assertRaises(ValueError):
            c.port_input(s)
        self.assertEqual(c.port_input(s,allow_test=True), '1:TEST:2')

    def test_alignment_uses_only_first_scene_edge(self):
        p = [row(f,1 if f<100 else 2) for f in range(1,150)]
        m = [row(f,0 if f<500 else 1 if f<608 else 2) for f in range(1,650)]
        self.assertEqual(c.boot_offset(p,m)['offset'],508)

    def test_missing_alignment_is_an_error(self):
        with self.assertRaises(RuntimeError):
            c.boot_offset([row(1)],[row(1)])

    def test_first_divergence_and_missing_port_ram(self):
        p = [row(f) for f in range(1,4)]
        m = [row(f) for f in range(1,6)]
        for f in range(1,4):
            m[f+1]['counter'] = f
        m[3]['actors'][2].update(active=True,hp=12)
        s = c.Scenario('test',3,[(0,0,0)]*4,[])
        r = c.compare(s,p,m,{'offset':2})
        self.assertEqual(r['first_field']['frame'],2)
        self.assertEqual(r['first_field']['mame_frame'],4)
        self.assertEqual(r['first_field']['field'],'actor[3].active')
        self.assertIn('ram',r['missing_port_fields'])
        self.assertEqual(r['status'],'DIVERGED')

    def test_mask_ignores_bios_but_keeps_rng(self):
        low, high = bytes(c.MIB), bytearray(c.MIB)
        mask = json.loads(Path('tools/scenario/ram_mask.json').read_text())
        before = c.ram_hashes(low,high,mask)
        high[0x248] = 1
        after = c.ram_hashes(low,high,mask)
        self.assertNotEqual(before['raw'], after['raw'])
        self.assertEqual(before['masked'], after['masked'])
        high[0x9957c] = 1
        self.assertNotEqual(before['masked'],c.ram_hashes(low,high,mask)['masked'])

    def test_bios_sega_pattern_is_not_an_attract_cycle(self):
        records = [row(1,0x53454741),row(2,0),row(3,1),row(4,2)]
        self.assertFalse(c.expectations(records,['attract_return'])['attract_return'])
        records += [row(5,3),row(6,1)]
        self.assertTrue(c.expectations(records,['attract_return'])['attract_return'])

    def test_screen_divergence_is_reported_with_missing_ram(self):
        p, m = [row(1)], [row(1)]
        p[0]['screen'] = {'width':320,'height':224,'sha256':'a'}
        m[0]['screen'] = {'width':352,'height':224,'sha256':'b'}
        r = c.compare(c.Scenario('test',1,[(0,0,0)]*2,[]),p,m,{'offset':0})
        self.assertEqual(r['status'],'DIVERGED')
        self.assertIsNotNone(r['first_screen'])
        self.assertIsNone(r['first_field'])

    def test_capture_gaps_are_errors(self):
        with self.assertRaises(RuntimeError):
            c.validate_frames([row(1),row(3)],3)

    def test_unavailable_fields_are_not_reported_as_equal(self):
        p, m = [row(1)], [row(1)]
        p[0]['rng'] = None
        r = c.compare(c.Scenario('test',1,[(0,0,0)]*2,[]),p,m,{'offset':0})
        self.assertEqual(r['status'],'INCOMPLETE')
        self.assertIn('rng',r['missing_port_fields'])


if __name__ == '__main__':
    unittest.main()
