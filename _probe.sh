#!/bin/bash
# 环境探测：git 状态 / remote / gh / token
OUT=/root/q30-term50-kit/src/BGShellBB10/_probe.out
exec > "$OUT" 2>&1

echo "=== whoami/host ==="
whoami; hostname; uname -a

echo "=== repo ==="
cd /root/q30-term50-kit/src/BGShellBB10 || exit 1
pwd
git rev-parse --is-inside-work-tree
git rev-parse --show-toplevel
echo "--- remote ---"
git remote -v
echo "--- branch ---"
git branch -a
echo "--- HEAD ---"
git log --oneline -3
echo "--- status count ---"
git status --short | wc -l

echo "=== gh cli ==="
which gh || echo "gh: NOT INSTALLED"
gh --version 2>/dev/null || true

echo "=== token env ==="
for v in GITHUB_TOKEN GH_TOKEN GITHUB_PAT; do
  val="${!v}"
  if [ -n "$val" ]; then echo "$v=SET(len=${#val})"; else echo "$v=EMPTY"; fi
done

echo "=== credential files ==="
ls -la ~/.git-credentials 2>/dev/null || echo "no ~/.git-credentials"
git config --global --get credential.helper || echo "no credential.helper"

echo "=== network to github ==="
curl -s -o /dev/null -w "api.github.com http=%{http_code} time=%{time_total}\n" --max-time 12 https://api.github.com || echo "curl failed"

echo "=== repo exists? ==="
curl -s --max-time 12 https://api.github.com/repos/chnak/BGShellBB10 | head -c 600
echo

echo "=== bar files ==="
ls -l /root/q30-term50-kit/src/BGShellBB10/BGShellBB10.bar 2>/dev/null
ls -l /root/q30-term50-kit/Term50.bar 2>/dev/null

echo "=== DONE ==="
