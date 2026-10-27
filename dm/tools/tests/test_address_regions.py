import unittest,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
RET=0xd65f03c0
NOP=0xd503201f
def imported(name):
    img=bytearray(0x3000)
    struct.pack_into('<I',img,4,0x20);img[0x20:0x24]=b'MOD0'
    struct.pack_into('<i',img,0x24,0x2100-0x20)
    for i,(tag,value) in enumerate(((6,0x2300),(5,0x2400),(0,0))):struct.pack_into('<qQ',img,0x2100+i*16,tag,value)
    struct.pack_into('<I',img,0x2318,1)
    data=b'\0'+name.encode()+b'\0';img[0x2400:0x2400+len(data)]=data
    return img

class RegionGates(unittest.TestCase):
    """The address region gates' sites (patch_nso.py "Address regions"): the SDK's symbols and the one store through the out pointer"""
    PRO, MOV_OUT, RETW = [0xa9bd7bfd, 0x910003fd], 0xaa0103f4, RET   # stp x29, x30, [sp, #-0x30]!; mov x29, sp; mov x20, x1; ret

    def test_the_one_store_through_the_out_pointer(self):
        from nso_code import out_store
        from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM
        md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
        def fn(*words):
            img = bytearray(0x200); struct.pack_into('<' + 'I' * len(words), img, 0x100, *words); return img, 4 * len(words)
        img, n = fn(*self.PRO, self.MOV_OUT, NOP, 0xf9000296, NOP, self.RETW)   # str x22, [x20]
        self.assertEqual(out_store(img, 0x100, n, md), (0x110, 22))
        img, n = fn(*self.PRO, self.MOV_OUT, 0xf9000295, 0xf9000296, self.RETW)   # two stores: no single answer
        with self.assertRaisesRegex(ValueError, '2 stores'): out_store(img, 0x100, n, md)
        img, n = fn(*self.PRO, NOP, 0xf9000296, self.RETW)   # no copy of x1
        with self.assertRaisesRegex(ValueError, 'out pointer'): out_store(img, 0x100, n, md)
        img, n = fn(*self.PRO, self.MOV_OUT, 0xf9000696, self.RETW)   # str x22, [x20, #8]: a field, not the out value
        with self.assertRaisesRegex(ValueError, '0 stores'): out_store(img, 0x100, n, md)

    def test_defined_symbols(self):
        from nso_syms import defined
        img = imported('_ZN2nn2os6detail11VammManager21AllocateAddressRegionEPmm')
        self.assertEqual(defined(img), {})   # imported, not defined
        struct.pack_into('<HQQ', img, 0x2318 + 6, 1, 0x500, 0x90)   # st_shndx, st_value, st_size
        self.assertEqual(defined(img), {'_ZN2nn2os6detail11VammManager21AllocateAddressRegionEPmm': (0x500, 0x90)})
        self.assertEqual(defined(bytearray(0x100)), {})   # no MOD0



if __name__ == '__main__': unittest.main()
