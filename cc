#!/usr/bin/env bash
# cc — Claude Code backend & model switcher
# Commands: direct | bedrock | vertex | foundry | sonnet | opus | status | help

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
    vertex)  echo "claude-opus-4@20250501" ;;
    *)       echo "claude-opus-4-6" ;;   # direct, foundry
  esac
}

_sonnet_model() {
  case "${1:-direct}" in
    bedrock) echo "us.anthropic.claude-sonnet-4-6-v1" ;;
    vertex)  echo "claude-sonnet-4@20250501" ;;
    *)       echo "claude-sonnet-4-6" ;;  # direct, foundry
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
        proj=$("$GCLOUD" config get-value project 2>/dev/null || echo "agentsforgood")
        echo "export CLAUDE_CODE_USE_VERTEX=1"
        echo "export CLOUD_ML_REGION=us-west1"
        echo "export ANTHROPIC_VERTEX_PROJECT_ID=${proj}"
        ;;
      foundry)
        local endpoint="${AZURE_FOUNDRY_ENDPOINT:-https://claud-assistant-resource.services.ai.azure.com/models}"
        echo "export ANTHROPIC_BASE_URL=${endpoint}"
        # Azure key: set AZURE_ANTHROPIC_API_KEY in your environment, or it falls back to ANTHROPIC_API_KEY
        if [[ -n "${AZURE_ANTHROPIC_API_KEY:-}" ]]; then
          echo "export ANTHROPIC_API_KEY=${AZURE_ANTHROPIC_API_KEY}"
        fi
        ;;
      direct)
        : # Uses ANTHROPIC_API_KEY already in env — no extra vars needed
        ;;
    esac
  } > "$ENV_FILE"
}

# ── Commands ─────────────────────────────────────────────────────────────────
_status() {
  local provider; provider=$(_active)
  local model;    model=$(_current_model)
  local is_sonnet=0
  [[ "$model" == *sonnet* ]] && is_sonnet=1

  echo ""
  echo "  ╔══ Claude Code Backend ══════════════════╗"
  case "$provider" in
    direct)  echo "  ║  Provider : Direct Anthropic API        ║" ;;
    bedrock) echo "  ║  Provider : Amazon Bedrock (us-west-1)  ║" ;;
    vertex)  echo "  ║  Provider : Google Vertex AI            ║" ;;
    foundry) echo "  ║  Provider : Azure AI Foundry            ║" ;;
  esac
  echo "  ║  Model    : ${model}"
  if [[ $is_sonnet -eq 1 ]]; then
    echo "  ║  ⚡ Sonnet fallback — run: cc opus"
  fi
  echo "  ╚═════════════════════════════════════════╝"
  echo ""
  echo "  Providers : cc direct | bedrock | vertex | foundry"
  echo "  Models    : cc opus (default) | sonnet (fallback)"
  echo ""
}

# ── Main ──────────────────────────────────────────────────────────────────────
CMD="${1:-status}"

case "$CMD" in
  direct|bedrock|vertex|foundry)
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

  sonnet)
    provider=$(_active)
    model=$(_sonnet_model "$provider")
    _set_model "$model"
    echo "  → Sonnet 4.6 on ${provider}  (open new Claude Code session to take effect)"
    ;;

  status|"")
    _status
    ;;

  help|-h|--help)
    echo ""
    echo "  cc — Claude Code backend switcher"
    echo ""
    echo "  PROVIDERS"
    echo "    cc direct     Direct Anthropic API (Teams account)  [default]"
    echo "    cc bedrock    Amazon Bedrock (us-west-1)"
    echo "    cc vertex     Google Vertex AI"
    echo "    cc foundry    Azure AI Foundry"
    echo ""
    echo "  MODEL TIER  (within current provider)"
    echo "    cc opus       Opus 4.6  — primary"
    echo "    cc sonnet     Sonnet 4.6 — use when rate-limited"
    echo ""
    echo "  INFO"
    echo "    cc            Show current provider + model"
    echo "    cc status     Same as above"
    echo ""
    echo "  RATE LIMIT WORKFLOW"
    echo "    Hit token limits?  →  cc sonnet  →  open new Claude Code session"
    echo "    Back to normal?    →  cc opus    →  open new Claude Code session"
    echo ""
    ;;

  *)
    echo "  Unknown: ${CMD}. Run 'cc help'." >&2
    exit 1
    ;;
esac
