#!/bin/bash
# 发布 .bar 到 GitHub Releases
#
# 用法:
#   GITHUB_TOKEN=ghp_xxx ./publish-release.sh
#   GITHUB_TOKEN=ghp_xxx ./publish-release.sh path/to/Other.bar v1.2.0
#
# token 需要 contents:write 权限（classic PAT 的 repo scope）。
# 不传 tag 时，自动从 bar-descriptor.xml 读 id/versionNumber/buildId 生成。

set -euo pipefail

REPO="${REPO:-chnak/BGShellBB10}"
HERE="$(cd "$(dirname "$0")" && pwd)"
BAR="${1:-$HERE/BGShellBB10.bar}"
TAG="${2:-}"

[ -n "${GITHUB_TOKEN:-}" ] || { echo "ERROR: 请设置 GITHUB_TOKEN 环境变量"; exit 1; }
[ -f "$BAR" ] || { echo "ERROR: 找不到 $BAR"; exit 1; }

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# --- 从 bar 里取 descriptor ---
unzip -p "$BAR" xyz/bar-descriptor.xml > "$TMP/desc.xml" 2>/dev/null \
  || unzip -p "$BAR" native/bar-descriptor.xml > "$TMP/desc.xml" 2>/dev/null \
  || { echo "ERROR: 无法从 bar 提取 bar-descriptor.xml"; exit 1; }

APP_ID=$(grep -oE '<id>[^<]*</id>'            "$TMP/desc.xml" | head -1 | sed -E 's/<\/?id>//g')
APP_NAME=$(grep -oE '<name>[^<]*</name>'      "$TMP/desc.xml" | head -1 | sed -E 's/<\/?name>//g')
VER=$(grep -oE '<versionNumber>[^<]*</versionNumber>' "$TMP/desc.xml" | head -1 | sed -E 's/<\/?versionNumber>//g')
BID=$(grep -oE '<buildId>[^<]*</buildId>'     "$TMP/desc.xml" | head -1 | sed -E 's/<\/?buildId>//g')
AUTHOR=$(grep -oE '<author>[^<]*</author>'    "$TMP/desc.xml" | head -1 | sed -E 's/<\/?author>//g')

[ -n "$TAG" ] || TAG="v${VER}.${BID}"

MD5=$(md5sum "$BAR" | cut -d' ' -f1)
SIZE=$(stat -c%s "$BAR")
FNAME=$(basename "$BAR")

echo "repo   : $REPO"
echo "app    : $APP_NAME ($APP_ID) by $AUTHOR"
echo "version: $VER build $BID  -> tag $TAG"
echo "file   : $FNAME  $SIZE B  md5=$MD5"
echo

# --- 创建 release（已存在则复用）---
python3 - "$TAG" "$APP_NAME" "$VER" "$BID" "$MD5" "$SIZE" "$FNAME" <<'PY' > "$TMP/rel.json"
import json, sys
tag, name, ver, bid, md5, size, fname = sys.argv[1:8]
body = f"""**{name} v{ver} (build {bid})**

构建：BB10 NDK 10.3.1 / Qt 4.8 / armle-v7
签名：devMode（安装前需设备开启开发模式并导入 debug token）

| 文件 | 大小 | MD5 |
|---|---|---|
| `{fname}` | {int(size):,} B | `{md5}` |

安装：`blackberry-deploy -installApp -device <IP> -password <PW> {fname}`
"""
print(json.dumps({"tag_name": tag, "name": f"{name} v{ver} (build {bid})",
                  "body": body, "draft": False, "prerelease": False},
                 ensure_ascii=False))
PY

HTTP=$(curl -sS -X POST \
  -H "Authorization: Bearer $GITHUB_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  "https://api.github.com/repos/$REPO/releases" \
  -d @"$TMP/rel.json" -o "$TMP/resp.json" -w '%{http_code}')

RID=$(python3 -c "import json;print(json.load(open('$TMP/resp.json')).get('id',''))" 2>/dev/null || true)

if [ "$HTTP" = "422" ]; then
  echo "tag $TAG 已存在，复用已有 release"
  curl -sS -H "Authorization: Bearer $GITHUB_TOKEN" \
    "https://api.github.com/repos/$REPO/releases/tags/$TAG" -o "$TMP/resp.json"
  RID=$(python3 -c "import json;print(json.load(open('$TMP/resp.json')).get('id',''))")
elif [ "$HTTP" != "201" ]; then
  echo "ERROR: 创建 release 失败 http=$HTTP"
  cat "$TMP/resp.json"; exit 1
fi

echo "release id: $RID"

# --- 上传 asset ---
UP=$(curl -sS -X POST \
  -H "Authorization: Bearer $GITHUB_TOKEN" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @"$BAR" \
  "https://uploads.github.com/repos/$REPO/releases/$RID/assets?name=$FNAME" \
  -o "$TMP/up.json" -w '%{http_code}')

python3 -c "
import json;d=json.load(open('$TMP/up.json'))
print('upload http=$UP state=', d.get('state'), d.get('size'), 'B')
print('url:', d.get('browser_download_url'))
" || cat "$TMP/up.json"

echo "done: https://github.com/$REPO/releases/tag/$TAG"
