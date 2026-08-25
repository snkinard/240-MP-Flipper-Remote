#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly script_dir
project_root="$(cd "${script_dir}/.." && pwd)"
readonly project_root
readonly requirements_file="${project_root}/requirements-ufbt.txt"

readonly sdk_version="1.4.3"
readonly sdk_api_version="87.1"
readonly sdk_target="f7"
readonly sdk_hardware="7"
readonly sdk_archive_name="flipper-z-f7-sdk-${sdk_version}.zip"
readonly sdk_url="https://update.flipperzero.one/builds/firmware/${sdk_version}/${sdk_archive_name}"
readonly sdk_sha256="2e89e70c6b5770440cbf02f2ca01a2f8804e05ddb77f66afdd23ed2584740c7f"

export UFBT_HOME="${UFBT_HOME:-${project_root}/.ufbt}"
readonly ufbt_python_dir="${UFBT_HOME}/python"
export PYTHONHASHSEED=0
readonly PYTHONHASHSEED

fail() {
    echo "uFBT toolchain failed: $*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "$1 is not on PATH"
}

require_local_ufbt() {
    [[ -f "${ufbt_python_dir}/ufbt/__main__.py" ]] ||
        fail "project-local uFBT is not installed; run '$0 setup'"
}

run_ufbt() {
    require_local_ufbt
    PYTHONNOUSERSITE=1 PYTHONPATH="${ufbt_python_dir}" python3 -m ufbt "$@"
}

locked_ufbt_version() {
    local version
    version="$(sed -n 's/^ufbt==\([^[:space:]]*\).*/\1/p' "${requirements_file}")"
    [[ -n "${version}" ]] || fail "cannot read the uFBT version from ${requirements_file}"
    printf '%s\n' "${version}"
}

archive_matches() {
    local archive_path="$1"
    python3 - "${archive_path}" "${sdk_sha256}" <<'PY'
import hashlib
from pathlib import Path
import sys

archive = Path(sys.argv[1])
expected = sys.argv[2]
if not archive.is_file():
    raise SystemExit(1)

digest = hashlib.sha256()
with archive.open("rb") as archive_file:
    for chunk in iter(lambda: archive_file.read(1024 * 1024), b""):
        digest.update(chunk)

actual = digest.hexdigest()
if actual != expected:
    print(f"SDK checksum mismatch for {archive}: expected {expected}, found {actual}", file=sys.stderr)
    raise SystemExit(1)
PY
}

verify_status() {
    local expected_ufbt status_json actual_api
    expected_ufbt="$(locked_ufbt_version)"
    status_json="$(run_ufbt status --json)" || fail "unable to read uFBT status"
    actual_api="$(run_ufbt -s get_apiversion | tail -n 1)" ||
        fail "unable to read the firmware API version"

    python3 - \
        "${expected_ufbt}" \
        "${sdk_api_version}" \
        "${sdk_target}" \
        "${status_json}" \
        "${actual_api}" <<'PY'
import json
import sys

expected_ufbt, expected_api, expected_target, status_json, actual_api = sys.argv[1:]

try:
    status = json.loads(status_json)
except json.JSONDecodeError as error:
    raise SystemExit(f"uFBT toolchain failed: invalid `ufbt status --json` output: {error}")

errors = []
for label, actual, expected in (
    ("uFBT version", str(status.get("ufbt_version", "<missing>")), expected_ufbt),
    ("SDK target", str(status.get("target", "<missing>")), expected_target),
    ("firmware API", actual_api.strip(), expected_api),
):
    if actual != expected:
        errors.append(f"{label}: expected {expected}, found {actual}")

if status.get("mode") != "local":
    errors.append(f"SDK source mode: expected local, found {status.get('mode', '<missing>')}")

if errors:
    raise SystemExit("uFBT toolchain failed:\n- " + "\n- ".join(errors))
PY
}

verify_installed_toolchain() {
    require_command python3
    require_local_ufbt
    verify_status
}

setup_toolchain() {
    local download_dir archive_path partial_path
    require_command python3
    require_command curl

    mkdir -p "${ufbt_python_dir}"
    python3 -m pip install \
        --disable-pip-version-check \
        --require-hashes \
        --target "${ufbt_python_dir}" \
        --upgrade \
        -r "${requirements_file}"
    require_local_ufbt

    download_dir="${UFBT_HOME}/download"
    archive_path="${download_dir}/${sdk_archive_name}"
    partial_path="${archive_path}.part"
    mkdir -p "${download_dir}"

    if ! archive_matches "${archive_path}"; then
        curl \
            --fail \
            --location \
            --retry 3 \
            --show-error \
            --silent \
            --output "${partial_path}" \
            "${sdk_url}"
        archive_matches "${partial_path}" || fail "downloaded SDK archive did not match its declared checksum"
        mv "${partial_path}" "${archive_path}"
    fi

    run_ufbt update --hw-target "${sdk_target}" --local "${archive_path}"
    verify_status
    printf 'Prepared uFBT %s with firmware/SDK %s, API %s, target %s/%s.\n' \
        "$(locked_ufbt_version)" \
        "${sdk_version}" \
        "${sdk_api_version}" \
        "${sdk_target}" \
        "${sdk_hardware}"
}

run_build() {
    local build_log build_status
    build_log="$(mktemp "${TMPDIR:-/tmp}/ufbt-build.XXXXXX")"
    trap 'rm -f "${build_log}"; trap - RETURN' RETURN

    set +e
    run_ufbt 2>&1 | tee "${build_log}"
    build_status="${PIPESTATUS[0]}"
    set -e
    [[ "${build_status}" -eq 0 ]] || return "${build_status}"

    grep -Fq "Target: ${sdk_hardware}, API: ${sdk_api_version}" "${build_log}" ||
        fail "build output did not confirm target ${sdk_hardware}, API ${sdk_api_version}"
    [[ -f "${project_root}/dist/mp_240_remote.fap" ]] || fail "build did not produce dist/mp_240_remote.fap"
}

usage() {
    cat <<'EOF'
Usage: scripts/ufbt-toolchain.sh setup|verify|lint|build|launch

  setup   Install hash-locked uFBT into project-local state and prepare the SDK.
  verify  Confirm the installed uFBT version, SDK source, target, and API.
  lint    Verify the toolchain, then lint the application sources.
  build   Verify the toolchain, build the FAP, and confirm target/API metadata.
  launch  Build and verify the FAP, then upload and start it over USB.
EOF
}

cd "${project_root}"

case "${1:-}" in
setup)
    setup_toolchain
    ;;
verify)
    verify_installed_toolchain
    ;;
lint)
    verify_installed_toolchain
    run_ufbt lint
    ;;
build)
    verify_installed_toolchain
    run_build
    ;;
launch)
    verify_installed_toolchain
    run_build
    run_ufbt launch
    ;;
-h|--help|help)
    usage
    ;;
*)
    usage >&2
    exit 2
    ;;
esac
