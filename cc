#!/usr/bin/env bash
# cc — Claude Code backend & model switcher
# Commands: teams | direct | bedrock | vertex | foundry | opus | opus45 | sonnet | sonnet45 | haiku | check | status | help

CONFIG_DIR="${HOME}/.config/claude-code"
ACTIVE_FILE="${CONFIG_DIR}/active-backend"
ENV_FILE="${CONFIG_DIR}/env.sh"
SETTINGS="${HOME}/.claude/settings.json"
GCLOUD="${HOME}/google-cloud-sdk/bin/gcloud"

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
    bedrock) echo "us.anthropic.claude-sonnet-4-6-v1" ;;
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
    echo "unset CLAUDE_CODE_USE_BEDROCK CLAUDE_CODE_USE_VERTEX ANTHROPIC_BASE_URL"
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
        local endpoint="${AZURE_FOUNDRY_ENDPOINT:-https://YOUR_RESOURCE.services.ai.azure.com/models}"
        echo "export ANTHROPIC_BASE_URL=${endpoint}"
        # Azure key: set AZURE_ANTHROPIC_API_KEY in your environment, or it falls back to ANTHROPIC_API_KEY
        if [[ -n "${AZURE_ANTHROPIC_API_KEY:-}" ]]; then
          echo "export ANTHROPIC_API_KEY=${AZURE_ANTHROPIC_API_KEY}"
        fi
        ;;
      direct)
        : # Uses ANTHROPIC_API_KEY already in env — no extra vars needed
        ;;
      teams)
        # OAuth-based Claude Teams — clear API key so Claude Code uses browser login
        echo "unset ANTHROPIC_API_KEY"
        ;;
    esac
  } > "$ENV_FILE"
}

_check_one() {
  local name="$1" code="$2"
  case "$code" in
    200) printf "  ✓ %-28s ready\n" "$name" ;;
    429) printf "  ~ %-28s needs quota\n" "$name" ;;
    401|403) printf "  ✗ %-28s auth failed (%s)\n" "$name" "$code" ;;
    404) printf "  ✗ %-28s not enabled\n" "$name" ;;
    *)   printf "  ? %-28s HTTP %s\n" "$name" "$code" ;;
  esac
}

_check() {
  echo ""
  echo "  cc check — provider connectivity"
  echo "  ────────────────────────────────────────"

  # teams
  printf "  - %-28s OAuth (use 'claude' to verify)\n" "teams"

  # direct
  if [[ -n "${ANTHROPIC_API_KEY:-}" ]]; then
    local code
    code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 10 \
      https://api.anthropic.com/v1/messages \
      -H "x-api-key: ${ANTHROPIC_API_KEY}" \
      -H "anthropic-version: 2023-06-01" \
      -H "content-type: application/json" \
      -d '{"model":"claude-opus-4-6","max_tokens":1,"messages":[{"role":"user","content":"hi"}]}')
    _check_one "direct" "$code"
  else
    printf "  - %-28s skipped (no API key)\n" "direct"
  fi

  # bedrock
  if command -v aws &>/dev/null; then
    local body tmpfile="/tmp/cc-check-bedrock.json"
    body=$(echo '{"anthropic_version":"bedrock-2023-05-31","max_tokens":1,"messages":[{"role":"user","content":"hi"}]}' | base64 -w0)
    if aws bedrock-runtime invoke-model --region us-west-1 \
      --model-id us.anthropic.claude-opus-4-6-v1 \
      --content-type "application/json" --accept "application/json" \
      --body "$body" "$tmpfile" >/dev/null 2>&1; then
      printf "  ✓ %-28s ready\n" "bedrock"
    else
      printf "  ✗ %-28s failed\n" "bedrock"
    fi
    rm -f "$tmpfile"
  else
    printf "  - %-28s skipped (no aws CLI)\n" "bedrock"
  fi

  # vertex — probe multiple models
  local vtx_token vtx_project
  vtx_token=$("$GCLOUD" auth print-access-token 2>/dev/null || echo "")
  vtx_project=$("$GCLOUD" config get-value project 2>/dev/null || echo "")
  if [[ -n "$vtx_token" && -n "$vtx_project" ]]; then
    local vtx_region="us-east5"
    local vtx_model code
    for vtx_model in claude-opus-4-6 claude-sonnet-4-6 claude-opus-4-5@20251101 claude-haiku-4-5@20251001; do
      code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 10 \
        "https://${vtx_region}-aiplatform.googleapis.com/v1/projects/${vtx_project}/locations/${vtx_region}/publishers/anthropic/models/${vtx_model}:streamRawPredict" \
        -H "Authorization: Bearer ${vtx_token}" \
        -H "Content-Type: application/json" \
        -d '{"anthropic_version":"vertex-2023-10-16","max_tokens":1,"messages":[{"role":"user","content":"hi"}]}')
      _check_one "vertex:${vtx_model%%@*}" "$code"
    done
  else
    printf "  - %-28s skipped (no gcloud auth)\n" "vertex"
  fi

  # foundry
  local fnd_endpoint="${AZURE_FOUNDRY_ENDPOINT:-${ANTHROPIC_BASE_URL:-}}"
  local fnd_key="${AZURE_ANTHROPIC_API_KEY:-${ANTHROPIC_API_KEY:-}}"
  if [[ -n "$fnd_endpoint" && -n "$fnd_key" ]]; then
    local code
    code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 10 \
      "${fnd_endpoint}" \
      -H "api-key: ${fnd_key}" \
      -H "content-type: application/json" \
      -d '{"model":"claude-opus-4-6","max_tokens":1,"messages":[{"role":"user","content":"hi"}]}')
    _check_one "foundry" "$code"
  else
    printf "  - %-28s skipped (no endpoint/key)\n" "foundry"
  fi
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
  echo "  Providers : cc teams | direct | bedrock | vertex | foundry"
  echo "  Models    : cc opus | opus45 | sonnet | sonnet45 | haiku"
  echo ""
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
    echo "    Env vars written to: ${ENV_FILE}"
    echo "    Run: source ~/.config/claude-code/env.sh   (or open a new terminal)"
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

  status|"")
    _status
    ;;

  help|-h|--help)
    echo ""
    echo "  cc — Claude Code backend switcher"
    echo ""
    echo "  PROVIDERS"
    echo "    cc teams      Claude Teams (OAuth browser login)  [default]"
    echo "    cc direct     Direct Anthropic API (API key)"
    echo "    cc bedrock    Amazon Bedrock (us-west-1)"
    echo "    cc vertex     Google Vertex AI"
    echo "    cc foundry    Azure AI Foundry"
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
    echo ""
    echo "  RATE LIMIT WORKFLOW"
    echo "    Hit Opus limits?   →  cc opus45  →  open new session"
    echo "    Need speed?        →  cc sonnet  →  open new session"
    echo "    Need cheapest?     →  cc haiku   →  open new session"
    echo "    Back to primary?   →  cc opus    →  open new session"
    echo ""
    ;;

  *)
    echo "  Unknown: ${CMD}. Run 'cc help'." >&2
    exit 1
    ;;
esac
