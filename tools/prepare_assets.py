"""Convert original game art to linear ARGB4444 and resample effects for AICA."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import argparse,struct,subprocess,json,wave
ap=argparse.ArgumentParser();ap.add_argument('--assets',type=Path,required=True);ap.add_argument('--out',type=Path,default=Path('disc'));ap.add_argument('--adpcm-tool',type=Path,required=True);args=ap.parse_args()
out=args.out;out.mkdir(parents=True,exist_ok=True);records=[]
def texture(im,dest):
    im=im.convert('RGBA')
    im.putdata([(r,g,b,0 if (r,g,b)==(0,0,0) else a) for r,g,b,a in im.getdata()])
    w=max(8,1<<(im.width-1).bit_length());h=max(8,1<<(im.height-1).bit_length())
    if im.size!=(w,h):im=im.resize((w,h),Image.Resampling.LANCZOS)
    raw=bytearray()
    for r,g,b,a in im.getdata():
        if r==g==b==0:a=0
        raw+=struct.pack('<H',((a>>4)<<12)|((r>>4)<<8)|((g>>4)<<4)|(b>>4))
    dest.parent.mkdir(parents=True,exist_ok=True)
    dest.write_bytes(struct.pack('<4sII',b'DCTX',w,h)+raw)
    records.append({'path':str(dest.relative_to(out)),'width':w,'height':h,'vram_bytes':len(raw)})
for p in (args.assets/'game/data').rglob('*'):
    if p.suffix.lower()=='.png':texture(Image.open(p),out/'game/data'/str(p.relative_to(args.assets/'game/data')).lower().replace('.png','.dct'))
font=Image.new('RGBA',(256,256));draw=ImageDraw.Draw(font)
face=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',14)
for i in range(32,127):draw.text(((i%16)*16,(i//16)*16-2),chr(i),font=face,fill='white')
texture(font,out/'font.dct')
total_audio=0
for p in args.assets.rglob('*'):
    if p.suffix.lower()!='.wav':continue
    rel=str(p.relative_to(args.assets)).lower();dest=out/rel;dest.parent.mkdir(parents=True,exist_ok=True)
    with wave.open(str(p)) as wav: duration=wav.getnframes()/wav.getframerate()
    rate=min(22050,int(65000/max(duration,.001)))
    subprocess.run(['ffmpeg','-v','error','-y','-i',str(p),'-ac','1','-ar',str(rate),'-c:a','pcm_s16le',str(dest)],check=True)
    temp=dest.with_suffix('.adpcm.wav')
    subprocess.run([str(args.adpcm_tool),'-t',str(dest),str(temp)],stdout=subprocess.DEVNULL,check=True)
    temp.replace(dest)
    total_audio+=dest.stat().st_size
subprocess.run(['ffmpeg','-v','error','-y','-i',str(args.assets/'music3.ogg'),'-ac','2','-ar','22050','-af','asetpts=N/SR/TB','-f','s16le',str(out/'music.raw')],check=True)
report={'textures':records,'texture_vram_bytes':sum(x['vram_bytes'] for x in records),'converted_wav_bytes':total_audio,'effects_codec':'KOS Yamaha ADPCM mono up to 22050 Hz, at most 65000 samples','music':'streamed signed 16-bit stereo 22050 Hz','font':'DejaVu Sans; licence retained'}
(out/'asset-manifest.json').write_text(json.dumps(report,indent=2))
print('Textures:',len(records),'VRAM bytes:',report['texture_vram_bytes'],'WAV bytes:',total_audio)
