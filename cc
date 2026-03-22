#!/usr/bin/env bash
# cc — Claude Code backend & model switcher
# Commands: teams | direct | bedrock | vertex | foundry | ollama [MODEL] | opus | opus45 | sonnet | sonnet45 | haiku | check | status | setup-* | help

CONFIG_DIR="${HOME}/.config/claude-code"
ACTIVE_FILE="${CONFIG_DIR}/active-backend"
ENV_FILE="${CONFIG_DIR}/env.sh"
SETTINGS="${HOME}/.claude/settings.json"
GCLOUD=$(command -v gcloud 2>/dev/null || echo "${HOME}/google-cloud-sdk/bin/gcloud")

mkdir -p "$CONFIG_DIR"

# ── Model IDs per provider ───────────────────────────────────────────────────
_opus_model() {
  case "${1:-direct}" in
    bedrock) echo "us.anthropic.claude-opus-4-6-v1" ;;
    vertex)  echo "claude-opus-4-6" ;;
    *)       echo "claude-opus-4-6" ;;   # direct, foundry
  esac
}

_opus45_model() {
  case "${1:-direct}" in
    bedrock) echo "us.anthropic.claude-opus-4-5-20251101-v1:0" ;;
    vertex)  echo "claude-opus-4-5@20251101" ;;
    *)       echo "claude-opus-4-5-20251101" ;;  # direct, foundry
  esac
}

_sonnet_model() {
  case "${1:-direct}" in
    bedrock) echo "us.anthropic.claude-sonnet-4-6" ;;
    vertex)  echo "claude-sonnet-4-6" ;;
    *)       echo "claude-sonnet-4-6" ;;  # direct, foundry
  esac
}

_sonnet45_model() {
  case "${1:-direct}" in
    bedrock) echo "us.anthropic.claude-sonnet-4-5-20250929-v1:0" ;;
    vertex)  echo "claude-sonnet-4-5@20250929" ;;
    *)       echo "claude-sonnet-4-5-20250929" ;;  # direct, foundry
  esac
}

_haiku_model() {
  case "${1:-direct}" in
    bedrock) echo "us.anthropic.claude-haiku-4-5-20251001-v1:0" ;;
    vertex)  echo "claude-haiku-4-5@20251001" ;;
    *)       echo "claude-haiku-4-5-20251001" ;;  # direct, foundry
  esac
}

# ── Ollama config helpers ─────────────────────────────────────────────────────
_ollama_env() { echo "${CONFIG_DIR}/ollama.env"; }

_ollama_var() {
  local key="$1" default="${2:-}"
  local env_file; env_file=$(_ollama_env)
  if [[ -f "$env_file" ]]; then
    local val
    val=$(sed -n "s/^export *${key}=[\"']*\([^\"']*\)[\"']*/\1/p" "$env_file" | tail -1)
    [[ -n "$val" ]] && echo "$val" && return
  fi
  echo "$default"
}

_ollama_default_model()  { _ollama_var OLLAMA_MODEL "qwen3-coder"; }
_ollama_url()            { _ollama_var OLLAMA_URL "http://localhost:11434"; }
_ollama_opus_model()     { _ollama_var OLLAMA_OPUS_MODEL "$(_ollama_default_model)"; }
_ollama_sonnet_model()   { _ollama_var OLLAMA_SONNET_MODEL "$(_ollama_default_model)"; }
_ollama_haiku_model()    { _ollama_var OLLAMA_HAIKU_MODEL "$(_ollama_default_model)"; }

# ── Helpers ───────────────────────────────────────────────────────────────────
_active() { cat "$ACTIVE_FILE" 2>/dev/null || echo "direct"; }

_current_model() {
  python3 -c "import json; print(json.load(open('${SETTINGS}'))['model'])" 2>/dev/null || echo "unknown"
}

_set_model() {
  local model="$1"
  if [[ -f "$SETTINGS" ]]; then
    python3 - <<PYEOF
import json
with open('${SETTINGS}') as f: s = json.load(f)
s['model'] = '${model}'
with open('${SETTINGS}', 'w') as f: json.dump(s, f, indent=4)
PYEOF
    echo "  ✓ model → ${model}"
  else
    echo "  ⚠ ${SETTINGS} not found — set model manually with /model in Claude Code"
  fi
}

_write_env() {
  local provider="$1"
  {
    echo "# Claude Code active backend: ${provider} ($(date '+%Y-%m-%d %H:%M'))"
    echo "unset CLAUDE_CODE_USE_BEDROCK CLAUDE_CODE_USE_VERTEX ANTHROPIC_BASE_URL ANTHROPIC_AUTH_TOKEN"
    echo "unset ANTHROPIC_MODEL ANTHROPIC_DEFAULT_OPUS_MODEL ANTHROPIC_DEFAULT_SONNET_MODEL ANTHROPIC_DEFAULT_HAIKU_MODEL"
    echo "export CLAUDE_CODE_DISABLE_TERMINAL_TITLE=1  # Let shell set title (for hostname-in-title)"
    case "$provider" in
      bedrock)
        echo "export CLAUDE_CODE_USE_BEDROCK=1"
        echo "export AWS_REGION=us-west-1"
        ;;
      vertex)
        local proj
        proj=$("$GCLOUD" config get-value project 2>/dev/null || echo "YOUR_PROJECT")
        echo "export CLAUDE_CODE_USE_VERTEX=1"
        echo "export CLOUD_ML_REGION=us-east5"
        echo "export ANTHROPIC_VERTEX_PROJECT_ID=${proj}"
        ;;
      foundry)
        # Source saved credentials if available
        local foundry_env="${CONFIG_DIR}/foundry.env"
        if [[ -f "$foundry_env" ]]; then
          # shellcheck disable=SC1090
          source "$foundry_env"
        fi
        local endpoint="${AZURE_FOUNDRY_ENDPOINT:-https://YOUR_RESOURCE.services.ai.azure.com/models}"
        echo "export ANTHROPIC_BASE_URL=${endpoint}"
        if [[ -n "${AZURE_ANTHROPIC_API_KEY:-}" ]]; then
          echo "export ANTHROPIC_API_KEY=${AZURE_ANTHROPIC_API_KEY}"
        fi
        ;;
      direct)
        # Source saved API key if available
        local direct_env="${CONFIG_DIR}/direct.env"
        if [[ -f "$direct_env" ]]; then
          cat "$direct_env"
        fi
        ;;
      teams)
        # OAuth-based Claude Teams — clear API key so Claude Code uses browser login
        echo "unset ANTHROPIC_API_KEY"
        ;;
      ollama)
        # Local Ollama — Anthropic-compatible API (requires Ollama v0.14+)
        local url; url=$(_ollama_url)
        echo "unset ANTHROPIC_API_KEY"
        echo "export ANTHROPIC_BASE_URL=${url}"
        echo "export ANTHROPIC_AUTH_TOKEN=ollama"
        echo "export ANTHROPIC_MODEL=$(_ollama_default_model)"
        echo "export ANTHROPIC_DEFAULT_OPUS_MODEL=$(_ollama_opus_model)"
        echo "export ANTHROPIC_DEFAULT_SONNET_MODEL=$(_ollama_sonnet_model)"
        echo "export ANTHROPIC_DEFAULT_HAIKU_MODEL=$(_ollama_haiku_model)"
        ;;
    esac
  } > "$ENV_FILE"
  chmod 600 "$ENV_FILE"
}

_probe_http() {
  curl -s -o /dev/null -w "%{http_code}" --max-time 10 "$@" 2>/dev/null || echo "000"
}

_status_label() {
  case "$1" in
    200) echo "✓ Ready" ;;
    429) echo "~ Needs quota" ;;
    401|403) echo "✗ Auth failed" ;;
    404) echo "✗ Not enabled" ;;
    000) echo "✗ No response" ;;
    skip:*) echo "- ${1#skip:}" ;;
    *)   echo "? HTTP $1" ;;
  esac
}

_check() {
  # Collect results into arrays
  local names=() statuses=()

  # teams
  names+=("teams"); statuses+=("skip:OAuth (use 'claude')")

  # direct
  if [[ -n "${ANTHROPIC_API_KEY:-}" ]]; then
    local code
    code=$(_probe_http https://api.anthropic.com/v1/messages \
      -H "x-api-key: ${ANTHROPIC_API_KEY}" \
      -H "anthropic-version: 2023-06-01" \
      -H "content-type: application/json" \
      -d '{"model":"claude-opus-4-6","max_tokens":1,"messages":[{"role":"user","content":"hi"}]}')
    names+=("direct"); statuses+=("$code")
  else
    names+=("direct"); statuses+=("skip:No API key")
  fi

  # bedrock
  if command -v aws &>/dev/null; then
    local body tmpfile="/tmp/cc-check-bedrock.json"
    body=$(echo '{"anthropic_version":"bedrock-2023-05-31","max_tokens":1,"messages":[{"role":"user","content":"hi"}]}' | base64 -w0)
    if aws bedrock-runtime invoke-model --region us-west-1 \
      --model-id us.anthropic.claude-opus-4-6-v1 \
      --content-type "application/json" --accept "application/json" \
      --body "$body" "$tmpfile" >/dev/null 2>&1; then
      names+=("bedrock"); statuses+=("200")
    else
      names+=("bedrock"); statuses+=("000")
    fi
    rm -f "$tmpfile"
  else
    names+=("bedrock"); statuses+=("skip:No aws CLI")
  fi

  # vertex — probe multiple models
  local vtx_token vtx_project
  vtx_token=$("$GCLOUD" auth print-access-token 2>/dev/null || echo "")
  vtx_project=$("$GCLOUD" config get-value project 2>/dev/null || echo "")
  if [[ -n "$vtx_token" && -n "$vtx_project" ]]; then
    local vtx_region="us-east5"
    local vtx_model code
    for vtx_model in claude-opus-4-6 claude-sonnet-4-6 claude-opus-4-5@20251101 claude-haiku-4-5@20251001; do
      code=$(_probe_http \
        "https://${vtx_region}-aiplatform.googleapis.com/v1/projects/${vtx_project}/locations/${vtx_region}/publishers/anthropic/models/${vtx_model}:streamRawPredict" \
        -H "Authorization: Bearer ${vtx_token}" \
        -H "Content-Type: application/json" \
        -d '{"anthropic_version":"vertex-2023-10-16","max_tokens":1,"messages":[{"role":"user","content":"hi"}]}')
      names+=("vertex:${vtx_model%%@*}"); statuses+=("$code")
    done
  else
    names+=("vertex"); statuses+=("skip:No gcloud auth")
  fi

  # foundry
  local fnd_endpoint="${AZURE_FOUNDRY_ENDPOINT:-${ANTHROPIC_BASE_URL:-}}"
  local fnd_key="${AZURE_ANTHROPIC_API_KEY:-${ANTHROPIC_API_KEY:-}}"
  if [[ -n "$fnd_endpoint" && -n "$fnd_key" ]]; then
    local code
    code=$(_probe_http "${fnd_endpoint}" \
      -H "api-key: ${fnd_key}" \
      -H "content-type: application/json" \
      -d '{"model":"claude-opus-4-6","max_tokens":1,"messages":[{"role":"user","content":"hi"}]}')
    names+=("foundry"); statuses+=("$code")
  else
    names+=("foundry"); statuses+=("skip:No endpoint/key")
  fi

  # ollama
  local ollama_url; ollama_url=$(_ollama_url)
  local code
  code=$(_probe_http "${ollama_url}/v1/messages" \
    -H "x-api-key: ollama" \
    -H "anthropic-version: 2023-06-01" \
    -H "content-type: application/json" \
    -d '{"model":"'"$(_ollama_default_model)"'","max_tokens":1,"messages":[{"role":"user","content":"hi"}]}')
  names+=("ollama"); statuses+=("$code")

  # Render table
  local nw=28 sw=32
  echo ""
  printf "  ┌─%-${nw}s─┬─%-${sw}s─┐\n" "$(printf '%0.s─' $(seq 1 $nw))" "$(printf '%0.s─' $(seq 1 $sw))"
  printf "  │ %-${nw}s │ %-${sw}s │\n" "Provider" "Status"
  printf "  ├─%-${nw}s─┼─%-${sw}s─┤\n" "$(printf '%0.s─' $(seq 1 $nw))" "$(printf '%0.s─' $(seq 1 $sw))"
  local i
  for i in "${!names[@]}"; do
    local label
    label=$(_status_label "${statuses[$i]}")
    printf "  │ %-${nw}s │ %-${sw}s │\n" "${names[$i]}" "$label"
  done
  printf "  └─%-${nw}s─┴─%-${sw}s─┘\n" "$(printf '%0.s─' $(seq 1 $nw))" "$(printf '%0.s─' $(seq 1 $sw))"
  echo ""
}

# ── Setup commands ────────────────────────────────────────────────────────────
_setup_bedrock() {
  echo ""
  echo "  AWS Bedrock Setup"
  echo "  ─────────────────"

  # Check if aws CLI exists
  if ! command -v aws &>/dev/null; then
    echo "  ✗ AWS CLI not found. Install: https://aws.amazon.com/cli/"
    return 1
  fi

  # Check existing credentials
  if [[ -f ~/.aws/credentials ]] && grep -q aws_access_key_id ~/.aws/credentials 2>/dev/null; then
    echo "  Found existing ~/.aws/credentials"
    read -p "  Overwrite? [y/N] " -n 1 -r
    echo
    [[ ! $REPLY =~ ^[Yy]$ ]] && echo "  Aborted." && return 0
  fi

  read -p "  AWS Access Key ID: " aws_key
  read -s -p "  AWS Secret Access Key: " aws_secret
  echo
  read -p "  Region [us-west-1]: " aws_region
  aws_region="${aws_region:-us-west-1}"

  mkdir -p ~/.aws
  cat > ~/.aws/credentials <<EOF
[default]
aws_access_key_id = ${aws_key}
aws_secret_access_key = ${aws_secret}
EOF
  chmod 600 ~/.aws/credentials

  cat > ~/.aws/config <<EOF
[default]
region = ${aws_region}
output = json
EOF

  echo ""
  echo "  ✓ AWS credentials saved to ~/.aws/credentials"
  echo "  ✓ Region set to ${aws_region}"
  echo ""
  echo "  Run 'cc bedrock' to switch to Bedrock provider"
  echo ""
}

_setup_vertex() {
  echo ""
  echo "  Google Vertex AI Setup"
  echo "  ──────────────────────"

  if [[ ! -x "$GCLOUD" ]]; then
    echo "  ✗ gcloud not found at ${GCLOUD}"
    echo "  Install: https://cloud.google.com/sdk/docs/install"
    return 1
  fi

  echo "  Opening browser for Google Cloud authentication..."
  "$GCLOUD" auth login

  echo ""
  read -p "  GCP Project ID: " gcp_project
  "$GCLOUD" config set project "$gcp_project"

  echo ""
  echo "  ✓ Authenticated with Google Cloud"
  echo "  ✓ Project set to ${gcp_project}"
  echo ""
  echo "  Run 'cc vertex' to switch to Vertex AI provider"
  echo ""
}

_setup_foundry() {
  echo ""
  echo "  Azure AI Foundry Setup"
  echo "  ──────────────────────"

  read -p "  Azure endpoint URL (e.g. https://YOUR_RESOURCE.services.ai.azure.com/models): " azure_endpoint
  read -s -p "  Azure API Key: " azure_key
  echo

  # Save to env file that gets sourced
  local foundry_env="${CONFIG_DIR}/foundry.env"
  cat > "$foundry_env" <<EOF
# Azure AI Foundry credentials (sourced by cc foundry)
export AZURE_FOUNDRY_ENDPOINT="${azure_endpoint}"
export AZURE_ANTHROPIC_API_KEY="${azure_key}"
EOF
  chmod 600 "$foundry_env"

  echo ""
  echo "  ✓ Azure credentials saved to ${foundry_env}"
  echo ""
  echo "  Run 'cc foundry' to switch to Azure AI Foundry"
  echo ""
}

_setup_direct() {
  echo ""
  echo "  Direct Anthropic API Setup"
  echo "  ──────────────────────────"

  read -s -p "  Anthropic API Key: " api_key
  echo

  local direct_env="${CONFIG_DIR}/direct.env"
  cat > "$direct_env" <<EOF
# Direct Anthropic API key (sourced by cc direct)
export ANTHROPIC_API_KEY="${api_key}"
EOF
  chmod 600 "$direct_env"

  echo ""
  echo "  ✓ API key saved to ${direct_env}"
  echo ""
  echo "  Run 'cc direct' to switch to direct API"
  echo ""
}

_setup_teams() {
  echo ""
  echo "  Claude Teams Setup"
  echo "  ──────────────────"
  echo ""
  echo "  Teams uses OAuth browser login - no credentials needed!"
  echo ""
  echo "  Requirements:"
  echo "    • Claude Teams or Claude Pro subscription"
  echo "    • Browser access for OAuth flow"
  echo ""
  echo "  Run 'cc teams' to switch, then start Claude Code."
  echo "  A browser window will open for authentication."
  echo ""
}

_setup_ollama() {
  echo ""
  echo "  Ollama Setup (requires Ollama v0.14+)"
  echo "  ──────────────────────────────────────"
  echo ""

  # Check if ollama is installed
  if ! command -v ollama &>/dev/null; then
    echo "  ✗ Ollama not found. Install: https://ollama.com/download"
    return 1
  fi

  local version
  version=$(ollama --version 2>/dev/null | grep -oP '[\d.]+' | head -1)
  echo "  Found Ollama ${version:-unknown}"

  read -p "  Ollama URL [http://localhost:11434]: " ollama_url
  ollama_url="${ollama_url:-http://localhost:11434}"

  # List available models if Ollama is running
  echo ""
  if curl -s "${ollama_url}/api/tags" >/dev/null 2>&1; then
    echo "  Available models:"
    curl -s "${ollama_url}/api/tags" | python3 -c "
import json, sys
d = json.load(sys.stdin)
for m in d.get('models', []):
    size = m.get('size', 0) / 1e9
    print(f'    {m[\"name\"]:35s} {size:.1f} GB')
" 2>/dev/null || echo "    (could not list models)"
    echo ""
  else
    echo "  ⚠ Ollama not reachable at ${ollama_url} — start it with: ollama serve"
    echo ""
  fi

  read -p "  Default model [qwen3-coder]: " ollama_model
  ollama_model="${ollama_model:-qwen3-coder}"

  echo ""
  echo "  Tier mapping (optional — maps cc opus/sonnet/haiku to Ollama models)"
  echo "  Press Enter to use default model for all tiers."
  read -p "  Opus model [${ollama_model}]: " opus_model
  opus_model="${opus_model:-${ollama_model}}"
  read -p "  Sonnet model [${ollama_model}]: " sonnet_model
  sonnet_model="${sonnet_model:-${ollama_model}}"
  read -p "  Haiku model [${ollama_model}]: " haiku_model
  haiku_model="${haiku_model:-${ollama_model}}"

  local ollama_env="${CONFIG_DIR}/ollama.env"
  cat > "$ollama_env" <<EOF
# Ollama configuration (sourced by cc ollama)
export OLLAMA_URL="${ollama_url}"
export OLLAMA_MODEL="${ollama_model}"
export OLLAMA_OPUS_MODEL="${opus_model}"
export OLLAMA_SONNET_MODEL="${sonnet_model}"
export OLLAMA_HAIKU_MODEL="${haiku_model}"
EOF
  chmod 600 "$ollama_env"

  echo ""
  echo "  ✓ Ollama config saved to ${ollama_env}"
  echo ""
  echo "  Run 'cc ollama' to switch to Ollama"
  echo "  Run 'cc ollama MODEL' to switch and override the model"
  echo ""
}

# ── Commands ─────────────────────────────────────────────────────────────────
_status() {
  local provider; provider=$(_active)
  local model;    model=$(_current_model)
  echo ""
  echo "  ╔══ Claude Code Backend ══════════════════╗"
  case "$provider" in
    teams)   echo "  ║  Provider : Claude Teams (OAuth)        ║" ;;
    direct)  echo "  ║  Provider : Direct Anthropic API        ║" ;;
    bedrock) echo "  ║  Provider : Amazon Bedrock (us-west-1)  ║" ;;
    vertex)  echo "  ║  Provider : Google Vertex AI            ║" ;;
    foundry) echo "  ║  Provider : Azure AI Foundry            ║" ;;
    ollama)  echo "  ║  Provider : Ollama (local)               ║" ;;
  esac
  echo "  ║  Model    : ${model}"
  case "$model" in
    *opus-4-5*)   echo "  ║  ⚡ Opus 4.5 fallback — run: cc opus" ;;
    *sonnet-4-5*) echo "  ║  ⚡ Sonnet 4.5 fallback — run: cc opus" ;;
    *sonnet*)     echo "  ║  ⚡ Sonnet 4.6 fallback — run: cc opus" ;;
    *haiku*)      echo "  ║  ⚡ Haiku fallback — run: cc opus" ;;
  esac
  echo "  ╚═════════════════════════════════════════╝"
  echo ""
  echo "  Providers : cc teams | direct | bedrock | vertex | foundry | ollama"
  echo "  Models    : cc opus | opus45 | sonnet | sonnet45 | haiku"
  echo "  Diagnose  : cc check"
  echo ""
}

_status_full() {
  _status
  _check
}

# ── Main ──────────────────────────────────────────────────────────────────────
CMD="${1:-status}"

case "$CMD" in
  teams|direct|bedrock|vertex|foundry)
    provider="$CMD"
    model=$(_opus_model "$provider")
    echo "$provider" > "$ACTIVE_FILE"
    _write_env "$provider"
    _set_model "$model"
    echo ""
    echo "  → Switched to ${provider}"
    # If called via shell function wrapper (bashrc cc()), env is auto-sourced.
    # If called directly, remind user to source.
    if [[ "${CC_AUTO_SOURCE:-}" != "1" ]]; then
      echo "    Run: source ~/.config/claude-code/env.sh   (or open a new terminal)"
    fi
    echo ""
    ;;

  ollama)
    provider="ollama"
    # Optional model override: cc ollama deepseek-r1:70b
    if [[ -n "${2:-}" ]]; then
      ollama_env=$(_ollama_env)
      if [[ -f "$ollama_env" ]]; then
        sed -i "s|^export OLLAMA_MODEL=.*|export OLLAMA_MODEL=\"${2}\"|" "$ollama_env"
      else
        mkdir -p "$CONFIG_DIR"
        cat > "$ollama_env" <<EOF
export OLLAMA_URL="http://localhost:11434"
export OLLAMA_MODEL="${2}"
export OLLAMA_OPUS_MODEL="${2}"
export OLLAMA_SONNET_MODEL="${2}"
export OLLAMA_HAIKU_MODEL="${2}"
EOF
        chmod 600 "$ollama_env"
      fi
    fi
    ollama_model=$(_ollama_default_model)
    echo "$provider" > "$ACTIVE_FILE"
    _write_env "$provider"
    _set_model "$ollama_model"
    echo ""
    echo "  → Switched to Ollama (model: ${ollama_model})"
    echo "    URL: $(_ollama_url)"
    if [[ "${CC_AUTO_SOURCE:-}" != "1" ]]; then
      echo "    Run: source ~/.config/claude-code/env.sh   (or open a new terminal)"
    fi
    echo ""
    ;;

  opus)
    provider=$(_active)
    model=$(_opus_model "$provider")
    _set_model "$model"
    echo "  → Opus 4.6 on $(_active)"
    ;;

  opus45)
    provider=$(_active)
    model=$(_opus45_model "$provider")
    _set_model "$model"
    echo "  → Opus 4.5 on ${provider}  (open new Claude Code session to take effect)"
    ;;

  sonnet)
    provider=$(_active)
    model=$(_sonnet_model "$provider")
    _set_model "$model"
    echo "  → Sonnet 4.6 on ${provider}  (open new Claude Code session to take effect)"
    ;;

  sonnet45)
    provider=$(_active)
    model=$(_sonnet45_model "$provider")
    _set_model "$model"
    echo "  → Sonnet 4.5 on ${provider}  (open new Claude Code session to take effect)"
    ;;

  haiku)
    provider=$(_active)
    model=$(_haiku_model "$provider")
    _set_model "$model"
    echo "  → Haiku 4.5 on ${provider}  (open new Claude Code session to take effect)"
    ;;

  check)
    _check
    ;;

  setup-bedrock)
    _setup_bedrock
    ;;

  setup-vertex)
    _setup_vertex
    ;;

  setup-foundry)
    _setup_foundry
    ;;

  setup-direct)
    _setup_direct
    ;;

  setup-teams)
    _setup_teams
    ;;

  setup-ollama)
    _setup_ollama
    ;;

  setup)
    echo ""
    echo "  cc setup-teams     Info on Claude Teams OAuth login"
    echo "  cc setup-direct    Configure Anthropic API key"
    echo "  cc setup-bedrock   Configure AWS credentials for Bedrock"
    echo "  cc setup-vertex    Authenticate with Google Cloud"
    echo "  cc setup-foundry   Configure Azure AI Foundry endpoint"
    echo "  cc setup-ollama    Configure local Ollama instance"
    echo ""
    ;;

  status|"")
    _status
    ;;

  help|-h|--help)
    echo ""
    echo "  cc — Claude Code backend switcher"
    echo ""
    echo "  SETUP  (first-time configuration)"
    echo "    cc setup-teams     Info on Claude Teams OAuth"
    echo "    cc setup-direct    Configure Anthropic API key"
    echo "    cc setup-bedrock   Configure AWS credentials"
    echo "    cc setup-vertex    Authenticate with Google Cloud"
    echo "    cc setup-foundry   Configure Azure AI Foundry"
    echo "    cc setup-ollama    Configure local Ollama"
    echo ""
    echo "  PROVIDERS"
    echo "    cc teams      Claude Teams (OAuth browser login)  [default]"
    echo "    cc direct     Direct Anthropic API (API key)"
    echo "    cc bedrock    Amazon Bedrock (us-west-1)"
    echo "    cc vertex     Google Vertex AI"
    echo "    cc foundry    Azure AI Foundry"
    echo "    cc ollama     Ollama local models (v0.14+)"
    echo "    cc ollama MODEL  Switch + set model (e.g. cc ollama deepseek-r1:70b)"
    echo ""
    echo "  MODEL TIER  (within current provider)"
    echo "    cc opus       Opus 4.6   — primary"
    echo "    cc opus45     Opus 4.5   — fallback when 4.6 rate-limited"
    echo "    cc sonnet     Sonnet 4.6 — fast + capable"
    echo "    cc sonnet45   Sonnet 4.5 — extended thinking"
    echo "    cc haiku      Haiku 4.5  — fastest, cheapest"
    echo ""
    echo "  INFO"
    echo "    cc            Show current provider + model"
    echo "    cc status     Same as above"
    echo "    cc check      Test connectivity for all providers"
    echo "    cc scan       Claude model matrix (5 tiers × 5 providers)"
    echo "    cc version    Show version"
    echo ""
    echo "  RATE LIMIT WORKFLOW"
    echo "    Hit Opus limits?   →  cc opus45  →  open new session"
    echo "    Need speed?        →  cc sonnet  →  open new session"
    echo "    Need cheapest?     →  cc haiku   →  open new session"
    echo "    Back to primary?   →  cc opus    →  open new session"
    echo ""
    ;;

  scan)
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    exec python3 "${SCRIPT_DIR}/cc-scan" "$@"
    ;;

  version)
    echo "claude-code-switcher v1.0.0"
    ;;

  *)
    echo "  Unknown: ${CMD}. Run 'cc help'." >&2
    exit 1
    ;;
esac
