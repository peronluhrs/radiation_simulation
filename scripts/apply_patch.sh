#!/usr/bin/env bash
set -euo pipefail

PATCH_FILE="${1:-}"
BRANCH="$(git rev-parse --abbrev-ref HEAD)"

if [[ -z "${PATCH_FILE}" ]]; then
  echo "[apply_patch] Reading patch from stdin..."
  PATCH_FILE="$(mktemp)"
  cat > "${PATCH_FILE}"
fi

echo "[apply_patch] Applying patch on branch: ${BRANCH}"
git status --porcelain
git diff --staged > /dev/null

# sauvegarde
STAMP="$(date +%Y%m%d-%H%M%S)"
WORKDIR="codex_reports"
mkdir -p "${WORKDIR}"

echo "[apply_patch] git apply --index ${PATCH_FILE}"
if ! git apply --index "${PATCH_FILE}"; then
  echo "[apply_patch] Patch failed to apply. Showing context:"
  git apply --reject "${PATCH_FILE}" || true
  echo "[apply_patch] Please resolve *.rej and re-run."
  exit 2
fi

MSG="codex: apply patch ${STAMP}"
git commit -m "${MSG}" || true

# build & smoke
BUILD_OK=0
SMOKE_OK=0
{
  echo "## Build & Smoke Report (${STAMP})"
  echo
  echo "Branch: \`${BRANCH}\`"
  echo
  echo "### Build"
  if ./scripts/dev_build.sh; then
    echo "- Build: **OK**"
  else
    echo "- Build: **FAIL**"
    BUILD_OK=1
  fi
  echo
  echo "### Smoke"
  if ./scripts/smoke_test.sh; then
    echo "- Smoke: **OK**"
  else
    echo "- Smoke: **FAIL**"
    SMOKE_OK=1
  fi
  echo
  echo "### git status"
  git status -s
} | tee "${WORKDIR}/report-${STAMP}.md"

if [[ ${BUILD_OK} -ne 0 || ${SMOKE_OK} -ne 0 ]]; then
  echo "[apply_patch] Some step failed. See ${WORKDIR}/report-${STAMP}.md"
  exit 3
fi

echo "[apply_patch] Done. Report: ${WORKDIR}/report-${STAMP}.md"
