"""Extract the donor's GLZA sound container, retaining its original audio files."""
from pathlib import Path, PurePosixPath
import argparse, binascii, os, stat, struct, zipfile, zlib

def extract(archive, destination):
    destination=Path(destination).resolve()
    destination.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(archive) as index, open(archive,'rb') as stream:
        entries=index.infolist()
        for number,entry in enumerate(entries,1):
            name=PurePosixPath(entry.filename)
            if (name.is_absolute() or '..' in name.parts or ':' in entry.filename
                    or '\\' in entry.filename or stat.S_ISLNK(entry.external_attr>>16)):
                raise ValueError('Unsafe sound path: '+entry.filename)
            if entry.is_dir():continue
            target=(destination/entry.filename).resolve()
            if destination not in target.parents:raise ValueError('Sound path escapes output')
            if entry.flag_bits&1 or entry.compress_type not in (0,8):
                raise ValueError('Unsupported sound compression/encryption')
            if entry.file_size>64*1024*1024:raise ValueError('Oversized sound')
            stream.seek(entry.header_offset)
            header=stream.read(30)
            if len(header)!=30 or header[:4] not in (b'PK\x03\x04',b'QL\x04\x05'):
                raise ValueError('Unexpected GLZA header for '+entry.filename)
            filename_size,extra_size=struct.unpack_from('<HH',header,26)
            raw_name=stream.read(filename_size)
            expected=entry.filename.encode('utf-8' if entry.flag_bits&0x800 else 'cp437')
            if raw_name!=expected:raise ValueError('Sound local/central filenames differ')
            stream.seek(extra_size,1)
            target.parent.mkdir(parents=True,exist_ok=True)
            temporary=target.with_name(target.name+'.part')
            decoder=zlib.decompressobj(-15) if entry.compress_type==8 else None
            crc=size=0
            remaining=entry.compress_size
            try:
                with temporary.open('xb') as output:
                    while remaining:
                        block=stream.read(min(65536,remaining))
                        if not block:raise ValueError('Truncated GLZA payload')
                        remaining-=len(block)
                        decoded=decoder.decompress(block,entry.file_size-size+1) if decoder else block
                        size+=len(decoded)
                        if size>entry.file_size:raise ValueError('Sound exceeds declared size')
                        output.write(decoded);crc=binascii.crc32(decoded,crc)
                    if decoder and (not decoder.eof or decoder.unused_data or decoder.unconsumed_tail):
                        raise ValueError('Invalid compressed sound stream')
                    if size!=entry.file_size or (crc&0xffffffff)!=entry.CRC:
                        raise ValueError('Sound CRC/size mismatch: '+entry.filename)
                os.replace(temporary,target)
            finally:
                if temporary.exists():temporary.unlink()
            if number==1 or number%25==0 or number==len(entries):
                print('Audio extraction: %d/%d (%d%%)'%(number,len(entries),number*100//len(entries)),flush=True)
    return len(entries)

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('donor',type=Path)
    args=parser.parse_args()
    folder=args.donor/'gameloft/games/GloftN2HP'
    count=extract(folder/'sounds_hi.glza',folder/'sounds')
    (folder/'sounds/.complete').write_text(str(count)+'\n',encoding='ascii')
