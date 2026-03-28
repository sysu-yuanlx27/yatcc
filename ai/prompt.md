I want you to adopt a strict "Long-Running Agent Harness" workflow to build this project.

Please follow this exact protocol for all our interactions to manage context limits and ensure high-quality code generation.

### 1. THE STATE FILE (Memory)
First, check if a file named `agent_progress.md` exists.
- **If NO:** Create it. You will act as the "Initializer Agent". Analyze the current request/codebase and create a detailed, step-by-step implementation plan in this file. Use Markdown checkboxes (`- [ ]`).
- **If YES:** You are the "Coding Agent". Read `agent_progress.md` to restore your context. Do NOT rely solely on conversation history. This file is the source of truth.

### 2. THE WORKFLOW LOOP
For each interaction, you must follow this cycle:
1.  **LOAD:** Read `agent_progress.md` to identify the next unchecked task.
2.  **FOCUS:** Select *only* the next single atomic task. Do not try to do multiple steps at once.
3.  **EXECUTE:** Write the code for that specific task.
4.  **VERIFY:** 
    - Create or run a test script to verify this specific change works. 
    - Do not assume it works; prove it (e.g., `npm test`, `python manage.py test`, or a custom script).
5.  **COMMIT:** If verified, run `git add .` and `git commit -m "feat: [Task Name]..."`.
6.  **UPDATE:** specificly check the box in `agent_progress.md` to mark it as done and append a brief log of what was changed/decided.
7.  **PAUSE:** Stop and ask me for confirmation to proceed to the next task.

### 3. CONSTRAINTS
- **No Hallucination:** If you are unsure about a library or API, verify it by reading the docs or checking the environment first.
- **Atomic Progress:** Never mark a task as done in the progress file until the tests pass.
- **Self-Correction:** If a test fails, update the progress file with a "Blocked/Failed" note and attempt to fix it before moving on.

Now, please start by initializing or reading the `agent_progress.md` based on my request:
- 阅读图片中的内容，按要求完成对应的实验。
- 按照实验目录中对应文件夹（如 `task/1/`）下的 `README.md` 文件中的要求完成实验。
- 选用 `flex` 和 `bison` 完成对应部分的任务。
- 不要删改给定的代码，只按要求添加需要的内容。
- 不要使用 Git，只编辑代码。
