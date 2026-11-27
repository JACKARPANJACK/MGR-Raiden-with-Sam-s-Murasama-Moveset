"""Check dedicated native EFF/EST/EFT resources and the particle-only route."""
import hashlib,io,json,struct,sys,tempfile,unittest
from pathlib import Path
from PIL import Image
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from assets import dat_entries
from build_lightning_effects import ROOT,build,records
from inspect_sam_sequences import parse

class NativeLightningAssets(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.config=json.loads((ROOT/'assets/lightning/effects.json').read_text())
        cls.source=(ROOT/'local_assets/lightning/coreeff-source.dat').read_bytes()
        cls.textures=(ROOT/'local_assets/lightning/coreeff-source.dtt').read_bytes()
        cls.output=ROOT/'Release/GameData/pl/pl1c30.dat'
        outer=dict(dat_entries(cls.output.read_bytes()))
        cls.assert_outer=outer
        cls.eff=dict(dat_entries(outer['pl1c30.eff']))
        cls.eft=dict(dat_entries(dict(dat_entries(cls.output.with_suffix('.dtt').read_bytes()))['pl1c30.eft']))
    def test_dedicated_bank_and_complete_native_records(self):
        self.assertEqual(set(self.assert_outer),{'pl1c30.eff'})
        self.assertEqual(self.config['bank'],0x11c30)
        h=parse(self.eff['header.bxm'])
        self.assertEqual({int(e.get('id')) for e in h.find('est')},set(range(950,968)))
        hashes=set()
        for id in range(950,968):
            blob=self.eff[f'{id}.est'];hashes.add(hashlib.sha256(blob).digest())
            _,rows=records(blob)
            self.assertTrue(0<len(rows)<=256)
            delays=set();variants=set()
            for row in rows:
                self.assertEqual(set(row),{b'PART',b'MOVE',b'EMIF',b'TEX '})
                self.assertEqual(struct.unpack_from('<h',row[b'PART'][1],12)[0],0)
                # No external mesh/material selector or conditional parent
                # render flags from the old 316/3 donor may leak into this bank.
                self.assertEqual(struct.unpack_from('<I',row[b'PART'][1],8)[0],0)
                self.assertEqual(row[b'TEX '][1][44:48],bytes(4))
                fade_in,fade_out=struct.unpack_from('<hh',row[b'MOVE'][1],232)
                self.assertGreater(struct.unpack_from('<h',row[b'PART'][1],2)[0],fade_in+fade_out)
                self.assertEqual(struct.unpack_from('<h',row[b'TEX '][1],4)[0],self.config['texture_id'])
                self.assertEqual(row[b'TEX '][1][31],1)
                self.assertGreater(struct.unpack_from('<h',row[b'TEX '][1],28)[0],0)
                self.assertEqual(struct.unpack_from('<h',row[b'TEX '][1],36)[0],0)
                variants.add(row[b'TEX '][1][30])
                delays.add(struct.unpack_from('<h',row[b'EMIF'][1],8)[0])
            self.assertGreater(len(delays),1)
            self.assertEqual(variants,{0,1,2,3})
        self.assertEqual(len(hashes),18)
    def test_atlas_does_not_replace_stock_particles(self):
        retail=dict(dat_entries(dict(dat_entries(self.source))['core.eff']))
        self.assertNotIn(f"{self.config['texture_id']:03}.wta",retail)
        self.assertNotIn('000.wta',self.eff)
        conflicting=dict(self.config,texture_id=0)
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(ValueError):
                build(self.source,self.textures,Path(folder)/'bad.dat',conflicting)
    def test_four_native_dxt5_lightning_textures(self):
        table=self.eff[f"{self.config['texture_id']:03}.wta"];payload=self.eft[f"{self.config['texture_id']:03}.wtp"]
        self.assertEqual(table[:4],b'WTB\0')
        self.assertEqual(struct.unpack_from('<I',table,8)[0],4)
        hashes=set()
        for i in range(4):
            offset=struct.unpack_from('<I',table,32+i*4)[0]
            size=struct.unpack_from('<I',table,64+i*4)[0]
            data=payload[offset:offset+size]
            self.assertEqual(data[84:88],b'DXT5')
            image=Image.open(io.BytesIO(data)).convert('RGBA')
            self.assertEqual(image.size,(256,256))
            alpha=image.getchannel('A')
            self.assertEqual(alpha.getextrema(),(0,255))
            self.assertLess(sum(1 for p in alpha.getdata() if p>200),5000)
            self.assertGreater(sum(1 for p in alpha.getdata() if p>200),500)
            hashes.add(hashlib.sha256(data).digest())
        self.assertEqual(len(hashes),4)
    def test_native_game_thread_route_no_custom_renderer(self):
        sources=''.join((ROOT/n).read_text() for n in ('gui.cpp','SamElectricCombat.h','SamMovesetManager.h','dllmain.cpp','NativeLightningEffects.h'))
        self.assertNotIn('LightningWorldEffects',sources)
        self.assertNotIn('LightningRibbonRenderer',sources)
        self.assertNotIn('DrawPrimitive', (ROOT/'NativeLightningEffects.h').read_text())
        self.assertIn('QueuePreview', (ROOT/'gui.cpp').read_text())
        self.assertIn('0xA00C50',(ROOT/'NativeLightningBank.h').read_text())
        self.assertIn('0xA00D60',(ROOT/'NativeLightningBank.h').read_text())
        self.assertIn('13',(ROOT/'NativeLightningEffects.h').read_text())
        self.assertIn('9',(ROOT/'NativeLightningEffects.h').read_text())
        binding=(ROOT/'SamVisualEffects.h').read_text()
        self.assertIn('(info,anchor,-1,0)',binding)
        self.assertIn('0x9FFAC0',binding)
        self.assertIn('&part->m_LocalMatrix',binding)
    def test_miss_effect_and_deterministic_packing(self):
        _,rows=records(self.eff['967.est'])
        self.assertGreaterEqual(min(struct.unpack_from('<f',r[b'MOVE'][1],12)[0] for r in rows),7)
        with tempfile.TemporaryDirectory() as folder:
            target=Path(folder)/'pl1c30.dat'
            build(self.source,self.textures,target,self.config)
            self.assertEqual(target.read_bytes(),self.output.read_bytes())
            self.assertEqual(target.with_suffix('.dtt').read_bytes(),self.output.with_suffix('.dtt').read_bytes())
        for bad in (b'',b'EFF\0'+b'\xff'*24):
            with self.assertRaises(ValueError):records(bad)
if __name__=='__main__':unittest.main()
