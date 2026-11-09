"""리소스 검사 및 목록 생성."""
from pathlib import Path
import array, hashlib, json, re, struct, wave
ROOT=Path(__file__).resolve().parents[1]
def main():
    source=(ROOT/'GameResources.cpp').read_text(encoding='utf-8-sig')
    header=(ROOT/'GameResources.h').read_text(encoding='utf-8-sig')
    block=re.search(r'TexturePaths\s*=\s*\{(.*?)\};',source,re.S).group(1)
    paths=re.findall(r'"(resource/[^\"]+)"',block)
    enum=re.search(r'enum class GameTexture[^\{]*\{(.*?)\};',header,re.S).group(1)
    names=[n.strip() for n in enum.split(',') if n.strip() and n.strip()!='Count']
    if len(names)!=len(paths): raise RuntimeError('Texture enum/path count mismatch')
    entries=[]
    all_paths=re.findall(r'"(resource/[^\"]+)"',source)
    if len(all_paths)!=len(set(all_paths)): raise RuntimeError('Duplicate load path')
    for path in all_paths:
        file=ROOT/path; raw=file.read_bytes()
        entry={'path':path,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
        if file.suffix=='.png':
            if raw[:8]!=b'\x89PNG\r\n\x1a\n': raise RuntimeError('Invalid PNG: '+path)
            entry.update(kind='texture',key=names[paths.index(path)],width=struct.unpack('>I',raw[16:20])[0],height=struct.unpack('>I',raw[20:24])[0])
        else:
            with wave.open(str(file),'rb') as f:
                if f.getsampwidth()!=2 or f.getcomptype()!='NONE': raise RuntimeError('Expected PCM16: '+path)
                samples=array.array('h',f.readframes(f.getnframes())); peak=max(abs(n) for n in samples)
                if peak>=32767: raise RuntimeError('Clipping: '+path)
                entry.update(kind='sound',channels=f.getnchannels(),sampleRate=f.getframerate(),seconds=round(f.getnframes()/f.getframerate(),4),peakPCM=peak)
        entries.append(entry)
    manifest={'textures':len(paths),'sounds':len(entries)-len(paths),'resources':entries}
    (ROOT/'docs'/'ResourceManifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'PASS: {manifest["textures"]} textures, {manifest["sounds"]} PCM sounds.')
if __name__=='__main__': main()
