# cc -- Claude Code Backend & Model Switcher

Quick-reference card. Script lives at `~/bin/cc` (173 lines bash).

## Commands

```
cc                  Show current provider + model (same as cc status)
cc status           Same as above

cc direct           Switch to Direct Anthropic API (Teams account)
cc bedrock          Switch to Amazon Bedrock (us-west-1)
cc vertex           Switch to Google Vertex AI
cc foundry          Switch to Azure AI Foundry

cc opus             Switch model to Opus 4.6 (keep current provider)
cc sonnet           Switch model to Sonnet 4.6 (keep current provider)

cc help             Show built-in help
```

## Rate-Limit Workflow

```
Hit token limits?  -->  cc sonnet  -->  open new Claude Code session
Back to normal?    -->  cc opus    -->  open new Claude Code session
```

## How It Works

1. `cc <provider>` does three things:
   - Writes `~/.config/claude-code/active-backend` (persists which provider is active)
   - Generates `~/.config/claude-code/env.sh` (env vars for that provider)
   - Updates `model` in `~/.claude/settings.json` (so Claude Code picks it up)

2. `cc opus` / `cc sonnet` only changes the model within the current provider.

3. After switching, either:
   - `source ~/.config/claude-code/env.sh` in your current terminal, OR
   - Open a new terminal (which should source it automatically)

## File Layout

```
~/bin/cc                                 Main CLI script
~/.config/claude-code/
    active-backend                       One word: direct|bedrock|vertex|foundry
    env.sh                               Sourced by shell -- sets provider env vars
~/.claude/settings.json                  Claude Code settings (model field updated)
```

## Model IDs by Provider

| Provider | Opus 4.6                              | Sonnet 4.6                              |
|----------|---------------------------------------|-----------------------------------------|
| direct   | claude-opus-4-6                       | claude-sonnet-4-6                       |
| bedrock  | us.anthropic.claude-opus-4-6-v1       | us.anthropic.claude-sonnet-4-6-v1       |
| vertex   | claude-opus-4@20250501                | claude-sonnet-4@20250501                |
| foundry  | claude-opus-4-6                       | claude-sonnet-4-6                       |

## Provider Env Vars

| Provider | Key Variables                                                |
|----------|--------------------------------------------------------------|
| direct   | (none -- uses ANTHROPIC_API_KEY already in env)              |
| bedrock  | CLAUDE_CODE_USE_BEDROCK=1, AWS_REGION=us-west-1              |
| vertex   | CLAUDE_CODE_USE_VERTEX=1, CLOUD_ML_REGION, VERTEX_PROJECT_ID |
| foundry  | ANTHROPIC_BASE_URL=<azure endpoint>                          |

## Provider Status

- **Direct** -- Working. Uses Teams account API key.
- **Bedrock** -- Working. IAM user `associate`, account 838517664017.
- **Vertex** -- Pending Opus 4.6 model access approval. Project: agentsforgood.
- **Azure Foundry** -- Pending approval. Endpoint configured.

## Legacy Script

There's also `~/Projects/lit-rpg-fantasy-voice/scripts/claude-provider.sh` --
the older version that must be `source`d. The `~/bin/cc` script supersedes it.
