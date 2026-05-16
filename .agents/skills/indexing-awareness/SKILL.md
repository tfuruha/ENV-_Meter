---
name: indexing-awareness
description: |
  Builds and queries a project-wide index of structure and dependencies to prevent incorrect assumptions about code context (RAG-like retrieval).
  Invoke in the following situations:
  - When starting work on an unfamiliar project or task
  - Before modifying, adding, or deleting files that may affect dependencies
  - When referencing a function, class, or symbol whose definition is not already confirmed
  - After changes to rules, workflows, or skills that may impact resolution or dependencies

  This skill dynamically selects an appropriate indexing or tracing routine based on the situation.
---

# Indexing Awareness Skill

Antigravity does not pre-index the repository, so the agent must **autonomously** understand the project structure and search for dependencies.
This skill provides a set of scripts that simulate Cursor's RAG (pre-indexing + relevant context search).

## Triggers

The agent should **proactively** execute the corresponding scripts in the following situations.
Do not wait for human instructions; make autonomous decisions according to the `Project Awareness` rule.

| Situation | Script to Execute |
|---|---|
| First time seeing the project or starting a new task | `index-structure.ps1` |
| Before modifying, adding, or deleting files | `trace-dependencies.ps1 -Target <target_file>` |
| Before referencing an unknown function, class, or symbol | `trace-dependencies.ps1 -Target <symbol_name>` |
| After changing rule, workflow, or skill configurations | `verify-structure.ps1` |

## Scripts

### 1. index-structure.ps1

Indexes the project's directory structure, file types, and sizes.

```powershell
# Learn the overall project index.
.\.agent\skills\indexing-awareness\scripts\index-structure.ps1

# Limit the search depth (default: 4).
.\.agent\skills\indexing-awareness\scripts\index-structure.ps1 -Depth 6

# Limit the search range.
.\.agent\skills\indexing-awareness\scripts\index-structure.ps1 -TargetDir src\
```

### 2. trace-dependencies.ps1

Searches for file or symbol dependencies in **both directions**.

```powershell
# File mode: trace dependencies in both directions.
.\.agent\skills\indexing-awareness\scripts\trace-dependencies.ps1 -Target src\utils\auth.ts

# Symbol mode: search for definitions and usages.
.\.agent\skills\indexing-awareness\scripts\trace-dependencies.ps1 -Target handleLogin

# Limit the search range.
.\.agent\skills\indexing-awareness\scripts\trace-dependencies.ps1 -Target UserService -SearchDir src\
```

**File mode:**
- Forward Dependencies — files that import/require the target file.
- Reverse Dependencies — files that the target file imports/requires.
- Config/Doc References — references from config files and documents.

**Symbol mode:**
- Definitions — where the symbol is defined (function, class, const, def, fn, etc.).
- Usages — where the symbol is used (word boundary match).
- Import References — references in import/require/from statements.

### 3. verify-structure.ps1

Verifies the consistency between rules, workflows, and skills in `.agent/` and the descriptions in `README.md`.

```powershell
.\.agent\skills\indexing-awareness\scripts\verify-structure.ps1
```

## Troubleshooting

- **`trace-dependencies.ps1` results are noisy:** Limit the search range using the second argument.
- **`index-structure.ps1` output is too long:** Restrict tree depth with `-Depth 2`.
- **"Missing in README" error in `verify-structure.ps1`:** Add appropriate descriptions to `README.md`.
