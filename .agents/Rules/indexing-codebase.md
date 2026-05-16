---
trigger: Model Decision
globs: ["**/*"]
---

# Project Structure Awareness

**Positioning:** This rule concretizes the principles of `2. Project Awareness` and `3. Senior Conduct` 
by defining **specific action procedures for indexing and dependency searching** 
of the project structure.

## 1. Index Before Act

- When starting a new task, first understand the project structure, **do not start based on guesses**.
- Specific methods:
    - Check directory structure using `ls`, `Get-ChildItem` , `find`, or `tree`.
    - **Execute `index-structure.ps1` from the `indexing-awareness` skill.**
- Identify entry points (e.g., `main`, `App`, `index`, `routes`) and recognize the architecture and tech stack.

## 2. Grep, Don't Guess

- **Do not invent nn-existent functions, types, or modules.** Verify existence with `grep`,`Select-String` if unsure.
- Specific methods:
    - `grep -rnI "symbol_name" --include="*.ext" .`
    - **Execute `trace-dependencies.ps1 <file or symbol>` from the `indexing-awareness` skill.**
- Check both **Forward (dependencies)** and **Reverse (dependents)** of the target file before editing.
- Warn beforehand if adding dependencies that risk circular references.

## 3. Verify After Change

- After adding, deleting, or renaming files, check with `grep` for any remaining references to old paths or names.
- If changing rule, workflow, or skill configurations, **verify document consistency with `verify-structure.ps1`**.
