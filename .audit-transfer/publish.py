import base64, hashlib, json, lzma, os, subprocess
from pathlib import Path

def git(*args, data=None):
    return subprocess.run(['git', *args], input=data, check=True, stdout=subprocess.PIPE).stdout

def publish(expected_repo, expected_hash):
    if os.environ['GITHUB_REPOSITORY'] != expected_repo:
        raise SystemExit('Wrong repository')
    if os.environ['GITHUB_REF'] != 'refs/heads/fix/audit-consolidated':
        raise SystemExit('Wrong target branch')
    if git('rev-parse', 'HEAD').decode().strip() != os.environ['GITHUB_SHA']:
        raise SystemExit('Checkout moved')
    if git('status', '--porcelain', '--untracked-files=all').strip():
        raise SystemExit('Dirty checkout')
    parts = sorted(Path('.audit-transfer').glob('*.b64'))
    raw = lzma.decompress(base64.b64decode(''.join(p.read_text().strip() for p in parts), validate=True))
    if hashlib.sha256(raw).hexdigest() != expected_hash:
        raise SystemExit('Payload digest mismatch')
    bundle = json.loads(raw)
    if bundle['repository'] != expected_repo or bundle['branch'] != 'fix/audit-consolidated':
        raise SystemExit('Payload destination mismatch')
    allowed = {str(p) for p in parts} | {'.github/workflows/apply-audit.yml', '.audit-transfer/publish.py'}
    drift = set(filter(None, git('diff', '--name-only', '-z', bundle['base'], 'HEAD').decode().split('\0')))
    if not drift <= allowed:
        raise SystemExit('Unexpected changes since audited baseline: ' + str(drift - allowed))
    def blob(path):
        if not path.exists():
            return None
        if not path.is_file() or path.is_symlink():
            raise SystemExit('Unsupported path type')
        data = path.read_bytes()
        return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
    paths = set()
    for rec in bundle['files']:
        path = Path(rec['path'])
        if path.is_absolute() or '..' in path.parts or path.parts[0] in ('.git', '.github', '.audit-transfer'):
            raise SystemExit('Unsafe payload path')
        if str(path) in paths:
            raise SystemExit('Duplicate payload path')
        paths.add(str(path))
        if blob(path) != rec['old']:
            raise SystemExit('Baseline blob mismatch: ' + str(path))
    patch = bundle['patch'].encode('utf-8')
    git('apply', '--check', '--index', '--whitespace=nowarn', '-', data=patch)
    git('apply', '--index', '--whitespace=nowarn', '-', data=patch)
    for rec in bundle['files']:
        if blob(Path(rec['path'])) != rec['new']:
            raise SystemExit('Applied blob mismatch: ' + rec['path'])
    staged = set(filter(None, git('diff', '--cached', '--name-only', '-z').decode().split('\0')))
    if staged != paths:
        raise SystemExit('Unexpected staged paths')
    git('config', 'user.name', 'github-actions[bot]')
    git('config', 'user.email', '41898282+github-actions[bot]@users.noreply.github.com')
    git('commit', '-m', bundle['message'])
    git('push', 'origin', 'HEAD:refs/heads/fix/audit-consolidated')
    print('Published', len(paths), 'verified paths at', git('rev-parse', 'HEAD').decode().strip())

if __name__ == '__main__':
    import sys
    publish(sys.argv[1], sys.argv[2])
