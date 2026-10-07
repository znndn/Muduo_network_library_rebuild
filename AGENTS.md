
## Communication and Learning Preferences

- Default to Simplified Chinese.
- For one-question-one-answer questions about teaching/learning points, simply address the user's question.
- When using dependency libraries to solve problems, you may install any tools globally or locally if necessary.

## Long-term Background

- User's name: 郑炜达.
- Windows account name: `zheng`.
- The user has an undergraduate education background in computer science and is currently in his senior (fourth) year.
- Long-term areas of interest: C++, Linux, algorithms, computer vision, trustworthy AI.
- Long-term commitment: electric guitar and music.
- Music preference: ACG, Japanese-style, and pop bands.
- The user is applying for a one-year taught master's program in Hong Kong. He is essentially certain to get into a top-five university, but entering a one-year program means the start of school coincides with the autumn recruitment season, so he is currently using the remaining time of his senior year to look for an internship.
- The user is preparing for an internship in the C++ backend direction and is somewhat anxious, because he is already a senior and does not yet have internship experience in this direction.
- The user is memorizing C++ interview Q&A / 八股文 following this link: https://notes.kamacoder.com/cpp/cpp-three-features.html#%E7%AE%80%E8%A6%81%E5%9B%9E%E7%AD%94
- The user is practicing algorithm problems following this link: https://programmercarl.com/algo/#%E6%8E%A8%E8%8D%90%E5%AD%A6%E4%B9%A0%E9%A1%BA%E5%BA%8F
- The user is learning MySQL following this link: https://leetcode.cn/studyplan/sql-free-50/ ; in the first semester of his junior year he studied the full content of databases along with the school, but has basically forgotten it all.
- The user is memorizing baguwen on the 四大件 following this link: https://notes.kamacoder.com/base/
- During his junior year the user spent a whole year on a research project, thus not spending enough time on C++ backend technology. His main goal for the senior year is to improve the necessary skills as quickly as possible and find a matching internship, ensuring that by the time he enrolls in graduate school he already has sufficient ability to take part in campus recruitment and employment.
- The user is trying to understand and independently reproduce the C++ muduo network library, referring to https://programmercarl.com/other/project_muduo.html . The user has no network programming experience and is fairly short on time. You need to assist the user in understanding all the architecture and code, and enable the user to independently reproduce the project. For the textual description of this project, please refer to the teaching in `D:\Obsidian Vault\6 工作笔记\代码随想录 C++项目\MuduoRaw` and `D:\Obsidian Vault\6 工作笔记\代码随想录 C++项目\网络库muduo-core.md`; these are also the user's learning materials for this project. For the specific code content, you can access `C:\AGENTS_WorkSpace\muduo-core\muduo-core-learning` (please `git pull` first). The project will be run on a remote Ubuntu server connected via VS Code, so please do not try to use WSL (unless explicitly requested).
- The following is a brief summary of the user's experience:
Edge computing and embedded car (software-hardware collaborative system):
Adopted a heterogeneous solution of "Kunpeng Orange Pi (openEuler + Python + YOLOv8 + OpenCV) + STM32 (PI control in C)", designed a multi-threaded queue to solve image acquisition/computation latency, and implemented real-time object detection, distance measurement, and autonomous motion control.
C++ high-performance system components (concurrent cache system):
Implemented a high-concurrency cache supporting LRU and an improved LFU (introducing an average access frequency threshold to evict overheated stale data), guaranteed thread safety based on mutexes and atomic operations, and completed high-ratio read/write stress testing.
Operations research optimization and engineering O&M:
In mathematical modeling, led the Python PuLP programming model and Monte Carlo stochastic scenario simulation; has experience in VDI clusters, standardized OS image creation, and basic server performance monitoring/O&M.
Won a provincial third prize in the Java group of the Blue Bridge Cup in his sophomore year.

## anymo Flashcards
- Default to a question-and-answer format.
- For the questions and answers of flashcards, unless explicitly specified otherwise, use the original wording given in the question as much as possible.
- Please insert appropriate line breaks between bullet points or paragraphs; you may refer to the line-breaking style of other baguwen flashcards. This takes priority over "use the original wording given in the question as much as possible".
- For code, please do not use code blocks in flashcards.
- If the user has not explicitly specified the content of the question and answer, and has not asked you to analyze it yourself, then by default use the title as the question and the body text as the answer; if there are attached photos, they must also be included.
- You may add cards into the folder of the corresponding section on your own (usually folders starting with "八股"). Do not add any cards into the "生活" (Life) and "已暂停" (Paused) folders, even if these folders match. If you are unsure, ask the user.

# Web Search
- If you need to access the international internet, Clash Verge is installed on this computer (usually running). If necessary (for example, the proxy node is unreachable or its region does not meet the requirement), you may switch the proxy node to a suitable one after notifying the user.

# Remote Server Proxy Tunnel (tencent-cpp)
- The remote Ubuntu server `tencent-cpp` (106.55.37.214, user `ubuntu`) cannot connect directly to the international internet, and relies on the local Clash Verge (127.0.0.1:7897) to provide a proxy through an SSH reverse tunnel; the remote `.bashrc` will point `http_proxy`/`https_proxy` to `http://127.0.0.1:7897` as needed.
- **Remote 7897 is an exclusive port; only one SSH connection can bind it at any given time.** Never add `RemoteForward 7897 127.0.0.1:7897` to any SSH alias in `C:\Users\zheng\.ssh\config`: VS Code Remote-SSH will open multiple connections/reconnect, and a later forward will fail silently after printing just one Warning — the remote proxy address is still there but the link is dead, manifesting as `proxyconnect tcp: dial tcp 127.0.0.1:7897: connect: connection refused`. This is exactly the root cause of the failure diagnosed on 2026-09-30.
- The tunnel is handled by an independent resident process, decoupled from the VS Code connection: script `C:\Users\zheng\tools\ssh-tunnel\tunnel-runner.ps1`, log `C:\Users\zheng\tools\ssh-tunnel\tunnel.log`, logon auto-start scheduled task `DSH-SSH-Tunnel-tencent-cpp`. It has `ExitOnForwardFailure=yes` together with an end-to-end health check (requesting generate_204 through the proxy every 30s); on disconnection/port occupation it retries every 15s, and as soon as the process dies it releases the remote port, leaving no zombie occupier.
- Remote-side three-step diagnostics: `ss -lntp | grep 7897` to see whether there is a listener, `curl -x http://127.0.0.1:7897 -o /dev/null -w '%{http_code}' https://www.gstatic.com/generate_204` to verify the link, and in an interactive shell `proxy_on` / `proxy_off` to manually toggle the proxy (when the tunnel is down, `.bashrc` automatically does not set the proxy, so commands go direct instead of getting connection refused).

# Workspace Artifacts
- After completing a task, unless the user explicitly requested before the artifact was created that it be kept, please directly delete useless intermediate artifacts, and place useful artifacts in a suitable location on this computer rather than necessarily `C:\AGENTS_WorkSpace`. If you are unsure, please communicate with the user.
- For files in the Obsidian directory, please do not directly add new files unless explicitly requested.
