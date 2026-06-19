document.addEventListener("DOMContentLoaded", function () {

    let username = prompt("Enter your display name:") || "";
    username = username.trim() || ("Guest#" + Math.floor(Math.random() * 9000 + 1000));
    document.getElementById("username-display").textContent = username;

    setStatus("Connecting...", false);

    const socket = new WebSocket("ws://10.182.40.145:12345");

    socket.onopen = function () {
        setStatus("Connected", true);
        addSystemMessage("✓ Connected to server.");
        addSystemMessage(`✓ Joined as "${username}".`);
    };

    socket.onclose = function () {
        setStatus("Disconnected", false);
        addSystemMessage("✗ Disconnected from server.");
    };

    socket.onerror = function () {
        setStatus("Error", false);
        addSystemMessage("✗ Connection error. Is the server running?");
    };

    socket.onmessage = function (event) {
        const sep   = event.data.indexOf("\x1F");
        const from  = sep !== -1 ? event.data.slice(0, sep)  : "?";
        const body  = sep !== -1 ? event.data.slice(sep + 1) : event.data;
        const isMine = from === username;

        addMessage(from, body, isMine);
    };

    function sendMessage() {
        const input = document.getElementById("msg");
        const text  = input.value.trim();

        if (!text) return;

        if (socket.readyState !== WebSocket.OPEN) {
            addSystemMessage("✗ Not connected. Cannot send message.");
            return;
        }

        socket.send(username + "\x1F" + text);
        input.value = "";
        input.focus();
    }

    document.getElementById("msg").addEventListener("keydown", function (e) {
        if (e.key === "Enter") sendMessage();
    });

    window.sendMessage = sendMessage;

    function addMessage(from, body, isMine) {
        const chat = document.getElementById("chat");
        const li   = document.createElement("li");

        li.className = isMine ? "msg msg--mine" : "msg msg--other";

        const meta = document.createElement("span");
        meta.className   = "msg-meta";
        meta.textContent = from;

        const bubble = document.createElement("span");
        bubble.className   = "msg-bubble";
        bubble.textContent = body;

        li.appendChild(meta);
        li.appendChild(bubble);
        chat.appendChild(li);
        chat.scrollTop = chat.scrollHeight;
    }

    function addSystemMessage(text) {
        const chat = document.getElementById("chat");
        const li   = document.createElement("li");
        li.className   = "msg msg--system";
        li.textContent = text;
        chat.appendChild(li);
        chat.scrollTop = chat.scrollHeight;
    }

    function setStatus(label, connected) {
        const dot  = document.querySelector(".status-dot");
        const text = document.querySelector(".status-text");

        if (dot) {
            dot.style.background  = connected ? "#22c55e" : "#ef4444";
            dot.style.boxShadow   = connected
                ? "0 0 6px #22c55e, 0 0 12px rgba(34,197,94,0.4)"
                : "0 0 6px #ef4444, 0 0 12px rgba(239,68,68,0.4)";
        }
        if (text) text.textContent = label;
    }
});