"""효과음 생성."""
from pathlib import Path
import math, random, wave, struct

RATE=22050
ROOT=Path(__file__).resolve().parents[1]/'resource'/'sound'
def save(name,duration,kind):
    rng=random.Random(26311038+kind)
    data=[]
    filtered=0.0
    for i in range(int(duration*RATE)):
        t=i/RATE
        p=t/duration
        noise=rng.uniform(-1,1)
        filtered=0.87*filtered+0.13*noise
        if kind==0: value=(noise*0.34+math.sin(2*math.pi*145*t)*0.23)*math.exp(-t*48)
        elif kind==1: value=(filtered*1.3+noise*0.12+math.sin(2*math.pi*(60*t-17*t*t))*0.22)*math.exp(-t*5)
        elif kind==2: value=(math.sin(2*math.pi*(220*t-130*t*t))*0.3+noise*0.1)*math.exp(-t*12)
        elif kind==3: value=math.sin(2*math.pi*(660 if p<0.5 else 880)*t)*0.22*math.sin(math.pi*p)**2
        elif kind==4:
            notes=(523.25,659.25,783.99,1046.5)
            phase=p*4; value=math.sin(2*math.pi*notes[min(3,int(phase))]*t)*0.24*math.sin(math.pi*(phase%1))**2
        else: value=math.sin(2*math.pi*(280*t+380*t*t))*0.13*math.sin(math.pi*p)**2
        fade=min(1,t/0.003,(duration-t)/0.012)
        data.append(struct.pack('<h',round(max(-0.9,min(0.9,value*fade))*32767)))
    ROOT.mkdir(parents=True,exist_ok=True)
    with wave.open(str(ROOT/(name+'.wav')),'wb') as f:
        f.setparams((1,2,RATE,0,'NONE','not compressed')); f.writeframes(b''.join(data))
    print(name,len(data)/RATE)
if __name__=='__main__':
    for kind,(name,seconds) in enumerate((('shot',0.13),('explosion',0.8),('hurt',0.26),('pickup',0.3),('clear',1.2),('jump',0.18))):
        save(name,seconds,kind)
