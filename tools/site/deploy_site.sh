#!/usr/bin/env bash
# Build the project website and publish it to the nginx host.
#
# Usage: tools/site/deploy_site.sh [--ref main] [--yes]
#   --ref  git ref the site's GitHub links point to (default: main)
#   --yes  skip the confirmation prompt
# Host and web root come from the environment, with the project's values as
# defaults. The host has no rsync, so the build is copied with scp and
# installed with sudo. The previous site is kept in /var/www/html.prev.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "${here}/../.." && pwd)"
host="${SITE_HOST:-refonte-flightsw@34.138.247.79}"
url="${SITE_URL:-https://flightsw-datng31.duckdns.org/}"
webroot="${SITE_WEBROOT:-/var/www/html}"
ref="main"
assume_yes="no"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --ref) ref="$2"; shift 2 ;;
    --yes) assume_yes="yes"; shift ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

out="$(mktemp -d)"
trap 'rm -rf "${out}"' EXIT
python3 "${repo}/tools/site/build_site.py" --ref "${ref}" --out "${out}/site"

echo "Publish to ${url} (${host}:${webroot}), links to ref '${ref}'."
if [[ "${assume_yes}" != "yes" ]]; then
  read -r -p "Continue? [y/N] " answer
  [[ "${answer}" == "y" || "${answer}" == "Y" ]] || { echo "aborted"; exit 1; }
fi

tar -C "${out}/site" -cf "${out}/site.tar" .
scp -q "${out}/site.tar" "${host}:/tmp/hk-site.tar"
ssh "${host}" "set -e
  stage=\$(mktemp -d)
  tar -C \"\$stage\" -xf /tmp/hk-site.tar
  sudo rm -rf '${webroot}.prev'
  sudo cp -a '${webroot}' '${webroot}.prev'
  sudo find '${webroot}' -mindepth 1 -delete
  sudo cp -a \"\$stage\"/. '${webroot}'/
  sudo chown -R root:root '${webroot}'
  sudo find '${webroot}' -type d -exec chmod 755 {} +
  sudo find '${webroot}' -type f -exec chmod 644 {} +
  rm -rf \"\$stage\" /tmp/hk-site.tar"

code="$(curl -s -o /dev/null -w '%{http_code}' "${url}")"
echo "published; ${url} answers HTTP ${code}"
[[ "${code}" == "200" ]]
