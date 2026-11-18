"""Run focused ARM/runtime checks: python3 run_runtime_tests.py /path/to/game [/path/to/music]"""
import pathlib,subprocess,sys
import unicorn
root=pathlib.Path(__file__).resolve().parent
if len(sys.argv) not in (2,3):raise SystemExit(__doc__)
subprocess.run([sys.executable,str(root/'build_desktop.py')],check=True)
u=pathlib.Path(unicorn.__file__).resolve().parent
build=root/'.desktop-build'
objects=[str(p) for p in build.glob('*.o') if p.name!='main_probe.o']
for name in ['app_settings_test','sound_continuation_test','controls_labels_test']:
 target=build/name
 subprocess.run(['g++','-std=c++17','-O2','-I'+str(root),str(root/'tests'/(name+'.cpp'))]+objects+['-L'+str(u/'lib'),'-Wl,-rpath,'+str(u/'lib'),'-l:libunicorn.so.2','-lz','-ldl','-o',str(target)],check=True)
 subprocess.run([str(target),sys.argv[1]],check=True)

for name in ['soundtrack_test','bottom_render_test']:
    target=build/name
    subprocess.run(['g++','-std=c++17','-O2','-I'+str(root),str(root/'tests'/(name+'.cpp')),'-o',str(target)],check=True)
    fixtures=build/(name+'_data');fixtures.mkdir(exist_ok=True)
    subprocess.run([str(target),str(fixtures)],check=True)

worker=build/'music_worker_test'
subprocess.run(['g++','-std=c++17','-O2','-pthread','-I'+str(root/'tests/fake3ds'),'-I'+str(root),str(root/'tests/music_worker_test.cpp'),'-o',str(worker)],check=True)
subprocess.run([str(worker),sys.argv[2] if len(sys.argv)==3 else str(pathlib.Path(sys.argv[1]).resolve().parent/'music')],check=True)
