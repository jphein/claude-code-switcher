# cc -- Claude Code Backend & Model Switcher

Switch Claude Code between API providers and model tiers from the command line.

## Install

```bash
git clone https://github.com/jphein/claude-code-switcher.git ~/Projects/claude-code-switcher
cd ~/Projects/claude-code-switcher
./install.sh
source ~/.bashrc
```

This adds the project folder to your PATH.

## First-Time Setup

Configure credentials for the providers you want to use:

```bash
cc setup-teams      # Info on Claude Teams OAuth (no creds needed)
cc setup-direct     # Anthropic API key for direct API
cc setup-bedrock    # AWS credentials for Bedrock
cc setup-vertex     # Google Cloud auth for Vertex AI
cc setup-foundry    # Azure endpoint + key for AI Foundry
```

## Commands

```
cc                  Show current provider + model
cc status           Same as above

cc teams            Claude Teams (OAuth browser login)  [default]
cc direct           Direct Anthropic API (API key)
cc bedrock          Amazon Bedrock (us-west-1)
cc vertex           Google Vertex AI (us-east5)
cc foundry          Azure AI Foundry

cc opus             Opus 5.5   -- primary
cc opus45           Opus 4.5   -- legacy fallback
cc sonnet           Sonnet 5.5 -- fast + capable
cc sonnet45         Sonnet 4.5 -- legacy; Claude API retires it 2026-11-30
cc haiku            Haiku 5.5  -- fastest, cheapest

cc check            Test connectivity for all providers
cc scan             Full model matrix — test all 5 models × 5 providers
cc help             Show built-in help

cc setup-teams      Info on Claude Teams OAuth
cc setup-direct     Configure Anthropic API key
cc setup-bedrock    Configure AWS credentials
cc setup-vertex     Authenticate with Google Cloud
cc setup-foundry    Configure Azure AI Foundry

cc-test             Run local state tests (55 checks)
cc-api-test         Test actual API connectivity per provider
```

### ⚠️ `cc check`, `cc scan`, `cc-scan` and `cc-api-test` cost money: agents must not run them

Each of these sends a real one-token request ("hi", `max_tokens: 1`) to every configured provider. On Bedrock that bills as AWS Marketplace with the default AWS credentials (`aws bedrock-runtime invoke-model … us.anthropic.claude-opus-5-5`), which promotional credits never cover. In October 2026 a loop of such probes billed Opus 4.6 on realm-cloud on two days: about 551 calls on 10-28 and about 58 on 10-29, seen as roughly 12 input tokens per output token in Cost Explorer.

- **Agents, status lines, prompts, cron jobs and timers:** never run `cc check`, `cc scan`, `cc-scan` or `cc-api-test`. To learn the active provider, read `~/.config/claude-code/active-backend` (and the model from `~/.config/claude-code/env.sh`), or run `cc status`, which reads local files only and makes no network call.
- **People:** run them by hand only, one at a time, when you are actually debugging a provider.

## Scan — Model Availability Matrix

`cc scan` tests every Claude model against every provider in parallel and shows a color-coded matrix:

```
  cc scan — Claude Model Availability Matrix

  Credentials:   OAuth    --    aws   token    key

               teams    direct  bedrock  vertex  foundry
  ──────────────────────────────────────────────────────
     Opus 5.5    --       --   ✓ 1285ms   --      --
     Opus 4.5    --       --   ✓  958ms   --      --
   Sonnet 5.5    --       --   ✓  742ms   --      --
   Sonnet 4.5    --       --   ✓ 1155ms   --      --
    Haiku 5.5    --       --   ✓  765ms   --      --
  ──────────────────────────────────────────────────────
```

Requires `httpx` (`pip install httpx`). Reads credentials from the same sources as `cc check` plus `~/.config/cloud-chat-assistant/config.json` as fallback.

## Rate-Limit Workflow

```
Hit Opus limits?   -->  cc opus45  -->  open new session
Need speed?        -->  cc sonnet  -->  open new session
Need cheapest?     -->  cc haiku   -->  open new session
Back to primary?   -->  cc opus    -->  open new session
```

## How It Works

1. `cc <provider>` writes three files:
   - `~/.config/claude-code/active-backend` -- persists provider choice
   - `~/.config/claude-code/env.sh` -- env vars for the provider
   - `~/.claude/settings.json` -- updates `model` field

2. Model commands (opus/opus45/sonnet/sonnet45/haiku) only change the model within the current provider.

3. After switching provider, source the env or open a new terminal:
   ```bash
   source ~/.config/claude-code/env.sh
   ```

## File Layout

```
~/Projects/claude-code-switcher/
    cc              Main script (added to PATH via install.sh)
    cc-scan         Model availability matrix (Python, requires httpx)
    cc-test         Local state test suite (55 checks)
    cc-api-test     API connectivity test
    install.sh      Adds project dir to PATH
    README.md       This doc

~/.config/claude-code/
    active-backend  Current provider: teams|direct|bedrock|vertex|foundry
    env.sh          Sourced by shell for provider env vars
    direct.env      Anthropic API key (from cc setup-direct)
    foundry.env     Azure endpoint + key (from cc setup-foundry)

~/.aws/credentials        AWS credentials (from cc setup-bedrock)
~/.claude/settings.json   Claude Code settings (model field)
```

## Model IDs by Provider

| Provider | Opus 5.5 | Opus 4.5 | Sonnet 5.5 | Sonnet 4.5 | Haiku 5.5 |
|----------|----------|----------|------------|------------|-----------|
| teams/direct | claude-opus-5-5 | claude-opus-4-5-20251101 | claude-sonnet-5-5 | claude-sonnet-4-5-20250929 | claude-haiku-5-5 |
| bedrock | us.anthropic.claude-opus-5-5 | us.anthropic.claude-opus-4-5-20251101-v1:0 | us.anthropic.claude-sonnet-5-5 | us.anthropic.claude-sonnet-4-5-20250929-v1:0 | us.anthropic.claude-haiku-5-5 |
| vertex | claude-opus-5-5 | claude-opus-4-5@20251101 | claude-sonnet-5-5 | claude-sonnet-4-5@20250929 | claude-haiku-5-5 |
| foundry | claude-opus-5-5 | claude-opus-4-5-20251101 | claude-sonnet-5-5 | claude-sonnet-4-5-20250929 | claude-haiku-5-5 |

## Provider Env Vars

| Provider | Variables Set |
|----------|-------------|
| teams    | Unsets ANTHROPIC_API_KEY (uses OAuth browser login) |
| direct   | None (uses ANTHROPIC_API_KEY from env) |
| bedrock  | CLAUDE_CODE_USE_BEDROCK=1, AWS_REGION=us-west-1 |
| vertex   | CLAUDE_CODE_USE_VERTEX=1, CLOUD_ML_REGION=us-east5, ANTHROPIC_VERTEX_PROJECT_ID |
| foundry  | ANTHROPIC_BASE_URL=\<azure endpoint\> |

## Terminal Flash Fix

Claude Code's spinner and progress updates can trigger terminal bells, causing screen flashes. Run once to fix:

```bash
./fix-terminal-flash.sh
```

This disables:
- GNOME Terminal cursor/text blinking
- System visual/audible bells
- Shell bell-style
- tmux bells (if using tmux)

## Prerequisites

- **teams**: Claude Teams/Pro subscription + browser for OAuth
- **direct**: ANTHROPIC_API_KEY in environment
- **bedrock**: AWS CLI + credentials in ~/.aws/credentials
- **vertex**: gcloud CLI + authenticated + Vertex AI API enabled + Claude quota
- **foundry**: Azure AI Foundry endpoint + API key

## License

AGPL-3.0-or-later © 2026 Jeffrey Pine Hein. See [LICENSE](LICENSE).
