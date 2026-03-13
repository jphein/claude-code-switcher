# cc -- Claude Code Backend & Model Switcher

Quick-reference card. Project at `~/Projects/claude-code-switcher/`

## Commands

```
cc                  Show current provider + model
cc status           Same as above

cc direct           Switch to Direct Anthropic API (Teams account)
cc bedrock          Switch to Amazon Bedrock (us-west-1)
cc vertex           Switch to Google Vertex AI
cc foundry          Switch to Azure AI Foundry

cc opus             Opus 4.6   — primary
cc opus45           Opus 4.5   — fallback when 4.6 rate-limited
cc sonnet           Sonnet 4.6 — fast + capable
cc sonnet45         Sonnet 4.5 — extended thinking
cc haiku            Haiku 4.5  — fastest, cheapest

cc help             Show built-in help
cc-test             Run local state tests (41 checks)
cc-api-test         Test actual API connectivity
```

## Rate-Limit Workflow

```
Hit Opus limits?   -->  cc opus45  -->  open new session
Need speed?        -->  cc sonnet  -->  open new session
Need cheapest?     -->  cc haiku   -->  open new session
Back to primary?   -->  cc opus    -->  open new session
```

## How It Works

1. `cc <provider>` does three things:
   - Writes `~/.config/claude-code/active-backend`
   - Generates `~/.config/claude-code/env.sh`
   - Updates `model` in `~/.claude/settings.json`

2. Model commands (opus/opus45/sonnet/sonnet45/haiku) only change the model.

3. After switching provider: `source ~/.config/claude-code/env.sh` or open new terminal.

## File Layout

```
~/Projects/claude-code-switcher/
    cc              Main script (symlinked to ~/bin/cc)
    cc-test         Local state test suite
    cc-api-test     API connectivity test
    README.md       This doc

~/.config/claude-code/
    active-backend  Current provider: direct|bedrock|vertex|foundry
    env.sh          Sourced by shell for provider env vars

~/.claude/settings.json   Claude Code settings (model field)
```

## Model IDs by Provider

| Provider | Opus 4.6                        | Opus 4.5                              | Sonnet 4.6                      | Sonnet 4.5                              | Haiku 4.5                               |
|----------|---------------------------------|---------------------------------------|---------------------------------|-----------------------------------------|-----------------------------------------|
| direct   | claude-opus-4-6                 | claude-opus-4-5-20251101              | claude-sonnet-4-6               | claude-sonnet-4-5-20250929              | claude-haiku-4-5-20251001               |
| bedrock  | us.anthropic.claude-opus-4-6-v1 | us.anthropic.claude-opus-4-5-20251101-v1:0 | us.anthropic.claude-sonnet-4-6-v1 | us.anthropic.claude-sonnet-4-5-20250929-v1:0 | us.anthropic.claude-haiku-4-5-20251001-v1:0 |
| vertex   | claude-opus-4@20250501          | claude-opus-4-5@20251101              | claude-sonnet-4@20250501        | claude-sonnet-4-5@20250929              | claude-haiku-4-5@20251001               |
| foundry  | claude-opus-4-6                 | claude-opus-4-5-20251101              | claude-sonnet-4-6               | claude-sonnet-4-5-20250929              | claude-haiku-4-5-20251001               |

## Provider Env Vars

| Provider | Key Variables                                                |
|----------|--------------------------------------------------------------|
| direct   | (none -- uses ANTHROPIC_API_KEY already in env)              |
| bedrock  | CLAUDE_CODE_USE_BEDROCK=1, AWS_REGION=us-west-1              |
| vertex   | CLAUDE_CODE_USE_VERTEX=1, CLOUD_ML_REGION, VERTEX_PROJECT_ID |
| foundry  | ANTHROPIC_BASE_URL=<azure endpoint>                          |

## Provider Status (2026-03-12)

- **Direct** -- Requires ANTHROPIC_API_KEY in env
- **Bedrock** -- WORKING. IAM user `associate`, account 838517664017
- **Vertex** -- Pending Opus model access approval. Project: agentsforgood
- **Azure Foundry** -- Pending approval. Endpoint configured
