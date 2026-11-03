#!/usr/bin/env python3
"""Build the HK-TM project website into build-site/ (static HTML, no runtime deps).

Each page under site/pages/ is an HTML fragment. This script wraps it in
site/layout.html, numbers its h2 sections into the "On this page" index,
inlines the SVG figures from site/figures/, and links repository paths to
GitHub. The technical report page is generated from docs/technical-report.md,
so the site cannot drift from the report in the repository.

Usage: tools/site/build_site.py [--ref main] [--out build-site]
Needs: Python 3 with the 'markdown' package (pip install markdown).
"""

import argparse
import datetime
import html
import pathlib
import re
import shutil
import subprocess

import markdown

REPO = pathlib.Path(__file__).resolve().parents[2]
SITE = REPO / "site"
GITHUB = "https://github.com/datnguyen31/flight-sw-sdlc"

# slug, tab label, page title, sources (repo paths the page is drawn from)
PAGES = [
    ("index", "Overview", "Housekeeping Telemetry Service: project overview",
     ["docs/technical-report.md", "docs/verification-report.md", "README.md"]),
    ("architecture", "Architecture", "Architecture and design",
     ["design/architecture.md", "design/fdir-design.md", "design/design-decisions.md",
      "src/app/tasks/hk_tasks.h"]),
    ("lifecycle", "Life cycle (V-model)", "Life cycle and the V-model",
     ["docs/software-development-plan.md", "verification/traceability-matrix.md",
      "docs/release-notes-v1.0.md"]),
    ("verification", "V&amp;V results", "Verification and validation results",
     ["docs/verification-report.md", "verification/system-test-results.md",
      "verification/timing-analysis.md", "verification/timing-measured.md",
      "verification/isvv-record.md"]),
    ("limitations", "Limitations", "Limitations and the path to flight",
     ["docs/verification-report.md", "docs/technical-report.md"]),
    ("report", "Technical report", "Technical report",
     ["docs/technical-report.md"]),
]

PATH_RE = re.compile(
    r"<code>((?:docs|design|requirements|verification|src|test|tools|ci|\.github)/"
    r"[\w./-]+)</code>")


def slugify(text):
    text = re.sub(r"<[^>]+>", "", html.unescape(text)).lower()
    text = re.sub(r"^\d+(\.\d+)*\.?\s*", "", text)
    return re.sub(r"[^a-z0-9]+", "-", text).strip("-")


def figure(name):
    svg = (SITE / "figures" / f"{name}.svg").read_text(encoding="utf-8")
    return re.sub(r"<\?xml[^>]*>\s*", "", svg).strip()


def link_paths(body, ref):
    def repl(m):
        path = m.group(1).rstrip(".")
        kind = "tree" if path.endswith("/") else "blob"
        return f'<a href="{GITHUB}/{kind}/{ref}/{path}"><code>{m.group(1)}</code></a>'
    return PATH_RE.sub(repl, body)


def number_sections(body):
    """Give every h2 an id and return the body with the section index."""
    toc = []

    def repl(m):
        attrs, text = m.group(1), m.group(2)
        found = re.search(r'id="([^"]+)"', attrs)
        ident = found.group(1) if found else slugify(text)
        if not found:
            attrs = f'{attrs} id="{ident}"'
        label = re.sub(r"^\d+\.\s*", "", re.sub(r"<[^>]+>", "", text))
        toc.append((ident, label))
        return f"<h2{attrs}>{text}</h2>"

    body = re.sub(r"<h2([^>]*)>(.*?)</h2>", repl, body, flags=re.S)
    return body, toc


ID_CELL_RE = re.compile(r"<td>((?:[A-Z]{1,4}-)+\d+[a-z]?)</td>")


def wrap_tables(body):
    """Scroll wrapper for wide tables; keep IDs such as SYS-1 on one line."""
    body = ID_CELL_RE.sub(r'<td class="nowrap">\1</td>', body)
    return re.sub(r"(<table.*?</table>)", r'<div class="table-wrap">\1</div>', body, flags=re.S)


def report_body():
    text = (REPO / "docs" / "technical-report.md").read_text(encoding="utf-8")
    text = re.sub(r"^# .*\n", "", text, count=1)
    # The repository diagram is Mermaid; the site uses the same figure as
    # the architecture page.
    text = re.sub(r"```mermaid\n.*?```", "<!--figure:architecture-->", text, count=1, flags=re.S)
    body = markdown.markdown(text, extensions=["tables", "fenced_code"])
    return body.replace("<p><!--figure:architecture--></p>", "<!--figure:architecture-->")


def nav_items(current):
    items = []
    for s, t, _, _ in PAGES:
        href = "./" if s == "index" else f"{s}.html"
        mark = ' aria-current="page"' if s == current else ""
        items.append(f'<li><a href="{href}"{mark}>{t}</a></li>')
    return "\n".join(items)


def build(ref, out):
    layout = (SITE / "layout.html").read_text(encoding="utf-8")
    commit = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=REPO,
                            capture_output=True, text=True, check=False).stdout.strip()
    today = datetime.date.today().isoformat()

    if out.exists():
        shutil.rmtree(out)
    shutil.copytree(SITE / "assets", out / "assets")

    for slug, tab, title, sources in PAGES:
        if slug == "report":
            body = report_body()
        else:
            body = (SITE / "pages" / f"{slug}.html").read_text(encoding="utf-8")
        body = re.sub(r"<!--figure:([\w-]+)-->",
                      lambda m: f'<div class="fig">{figure(m.group(1))}</div>', body)
        body = body.replace("{{repo}}", f"{GITHUB}/blob/{ref}")
        body = link_paths(body, ref)
        body = wrap_tables(body)
        body, toc = number_sections(body)

        tabs = nav_items(slug)
        pages = tabs
        toc_html = "\n".join(f'<li><a href="#{i}">{html.escape(l)}</a></li>' for i, l in toc)
        src_html = "\n".join(
            f'<li><a href="{GITHUB}/blob/{ref}/{p}"><code>{p}</code></a></li>' for p in sources)
        crumb = "" if slug == "index" else f'<span>/</span>{tab}'

        page = layout
        for key, val in {
            "title": title, "tab": tab, "tabs": tabs, "pages": pages, "toc": toc_html,
            "sources": src_html, "crumb": crumb, "body": body, "github": GITHUB,
            "ref": ref, "commit": commit or "unknown", "date": today,
        }.items():
            page = page.replace("{{" + key + "}}", val)
        (out / f"{slug}.html").write_text(page, encoding="utf-8")
    print(f"built {len(PAGES)} pages into {out} (ref {ref}, commit {commit})")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--ref", default="main", help="git ref the GitHub links point to")
    ap.add_argument("--out", default=str(REPO / "build-site"), help="output directory")
    args = ap.parse_args()
    build(args.ref, pathlib.Path(args.out).resolve())


if __name__ == "__main__":
    main()
