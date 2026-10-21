import io

p = '/root/q30-term50-kit/src/BGShellBB10/main.cpp'
s = io.open(p, encoding='utf-8').read()

broken = '''               << QString("/accounts/1000/appdata/com.q30.bgshellbb10/a    // The bundled ssh suite ships as assets; some installers drop the exec bit
    // from files inside a .bar, so make sure they stay runnable either way.
    {
        const char *tools[] = { "ssh", "scp", "sftp", "ssh-keygen" };
        for (unsigned i = 0; i < sizeof(tools) / sizeof(tools[0]); ++i) {
            QString tool = QString(cwd) + "/app/native/bin/" + tools[i];
            if (QFile::exists(tool))
                chmod(tool.toAscii().data(), 0755);
        }
    }

    // Init file descriptors opening proper devices
0; i < candidates.size(); ++i) {'''

fixed = '''               << QString("/accounts/1000/appdata/com.q30.bgshellbb10/assets/bgscjk.ttf");

    for (int i = 0; i < candidates.size(); ++i) {'''

assert broken in s, 'broken block not found'
s = s.replace(broken, fixed, 1)

anchor = '    qputenv("LD_LIBRARY_PATH", newPath.toAscii().data());'
assert anchor in s, 'anchor missing'
block = anchor + '''

    // The bundled ssh suite ships as assets; some installers drop the exec bit
    // from files inside a .bar, so make sure the tools stay runnable.
    {
        static const char *tools[] = { "ssh", "scp", "sftp", "ssh-keygen" };
        for (int i = 0; i < (int)(sizeof(tools) / sizeof(tools[0])); ++i) {
            const QString tool = QString(cwd) + "/app/native/bin/" + tools[i];
            if (QFile::exists(tool))
                chmod(tool.toAscii().data(), 0755);
        }
    }'''
s = s.replace(anchor, block, 1)

io.open(p, 'w', encoding='utf-8').write(s)
print('repaired')

# sanity: show the regions
lines = io.open(p, encoding='utf-8').read().splitlines()
for i, l in enumerate(lines):
    if 'bgscjk.ttf");' in l or 'static const char *tools[]' in l:
        print(i + 1, l.strip()[:90])
