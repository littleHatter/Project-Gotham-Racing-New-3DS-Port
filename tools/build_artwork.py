"""Convert the supplied artwork to ARGB sprites and a 48x48 launcher icon."""
from PIL import Image
from pathlib import Path
import sys
assets=Path(sys.argv[1]);root=Path(__file__).resolve().parents[1]
files={'splash':'PGRbottomscreen.png','promptA':'A.png','promptB':'B.png','promptStart':'START.png','loading':'PGR_loading.png'}
text=['#pragma once','#include <cstdint>','namespace PgrArt {']
for name,file in files.items():
 im=Image.open(assets/file).convert('RGBA');w,h=im.size
 text.append(f'constexpr unsigned {name}W={w},{name}H={h};\nstatic const uint32_t {name}[]={{')
 values=[(a<<24)|(r<<16)|(g<<8)|b for r,g,b,a in im.getdata()]
 for start in range(0,len(values),12):text.append(','.join(hex(v) for v in values[start:start+12])+',')
 text.append('};')
text.append('}');(root/'pgr_artwork.h').write_text('\n'.join(text)+'\n')
im=Image.open(assets/'PGRiconcandidate.png').convert('RGBA');bg=Image.new('RGBA',im.size,'black');bg.alpha_composite(im);bg.convert('RGB').resize((48,48),Image.Resampling.LANCZOS).save(root/'icon.png')
