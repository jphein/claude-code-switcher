# cc -- Claude Code Backend & Model Switcher

Switch Claude Code between API providers and model tiers from the command line.

## Install

```bash
./install.sh
```

Or manually:
```bash
ln -s "$(pwd)/cc" ~/bin/cc
ln -s "$(pwd)/cc-test" ~/bin/cc-test
ln -s "$(pwd)/cc-api-test" ~/bin/cc-api-test
```

## First-Time Setup

Configure credentials for the providers you want to use:

```bash
cc setup-bedrock    # AWS credentials for Bedrock
cc setup-vertex     # Google Cloud auth for Vertex AI
cc setup-foundry    # Azure endpoint + key for AI Foundry
cc setup-direct     # Anthropic API key for direct API
```

Teams provider uses OAuth browser login -- no setup needed.

## Commands

```
cc                  Show current provider + model
cc status           Same as above

cc teams            Claude Teams (OAuth browser login)  [default]
cc direct           Direct Anthropic API (API key)
cc bedrock          Amazon Bedrock (us-west-1)
cc vertex           Google Vertex AI (us-east5)
cc foundry          Azure AI Foundry

cc opus             Opus 4.6   -- primary
cc opus45           Opus 4.5   -- fallback when 4.6 rate-limited
cc sonnet           Sonnet 4.6 -- fast + capable
cc sonnet45         Sonnet 4.5 -- extended thinking
cc haiku            Haiku 4.5  -- fastest, cheapest

cc check            Test connectivity for all providers
cc help             Show built-in help

cc setup-bedrock    Configure AWS credentials
cc setup-vertex     Authenticate with Google Cloud
cc setup-foundry    Configure Azure AI Foundry
cc setup-direct     Configure Anthropic API key

cc-test             Run local state tests (48 checks)
cc-api-test         Test actual API connectivity per provider
```

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
    cc              Main script (symlinked to ~/bin/cc)
    cc-test         Local state test suite (48 checks)
    cc-api-test     API connectivity test
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

| Provider | Opus 4.6 | Opus 4.5 | Sonnet 4.6 | Sonnet 4.5 | Haiku 4.5 |
|----------|----------|----------|------------|------------|-----------|
| teams/direct | claude-opus-4-6 | claude-opus-4-5-20251101 | claude-sonnet-4-6 | claude-sonnet-4-5-20250929 | claude-haiku-4-5-20251001 |
| bedrock | us.anthropic.claude-opus-4-6-v1 | us.anthropic.claude-opus-4-5-20251101-v1:0 | us.anthropic.claude-sonnet-4-6-v1 | us.anthropic.claude-sonnet-4-5-20250929-v1:0 | us.anthropic.claude-haiku-4-5-20251001-v1:0 |
| vertex | claude-opus-4-6 | claude-opus-4-5@20251101 | claude-sonnet-4-6 | claude-sonnet-4-5@20250929 | claude-haiku-4-5@20251001 |
| foundry | claude-opus-4-6 | claude-opus-4-5-20251101 | claude-sonnet-4-6 | claude-sonnet-4-5-20250929 | claude-haiku-4-5-20251001 |

## Provider Env Vars

| Provider | Variables Set |
|----------|-------------|
| teams    | Unsets ANTHROPIC_API_KEY (uses OAuth browser login) |
| direct   | None (uses ANTHROPIC_API_KEY from env) |
| bedrock  | CLAUDE_CODE_USE_BEDROCK=1, AWS_REGION=us-west-1 |
| vertex   | CLAUDE_CODE_USE_VERTEX=1, CLOUD_ML_REGION=us-east5, ANTHROPIC_VERTEX_PROJECT_ID |
| foundry  | ANTHROPIC_BASE_URL=\<azure endpoint\> |

## Prerequisites

- **teams**: Claude Teams/Pro subscription + browser for OAuth
- **direct**: ANTHROPIC_API_KEY in environment
- **bedrock**: AWS CLI + credentials in ~/.aws/credentials
- **vertex**: gcloud CLI + authenticated + Vertex AI API enabled + Claude quota
- **foundry**: Azure AI Foundry endpoint + API key
