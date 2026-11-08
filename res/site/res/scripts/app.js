import * as api from "./api.js";

const messageInput = document.getElementById("message-input");
const messageTemplate = document.getElementById("message-template");
const messageContainer = document.getElementById("message-container");

api.setConnectHandler(() => {
    cookieStore.get("user").then((userCookie) => {
        if (userCookie) {
            cookieStore.get("sessionID").then((sessionCookie) => {
                if (sessionCookie) {
                    api.authenticate(atob(userCookie.value), atob(sessionCookie.value));
                }
            });
        }
    });
});
api.setAuthHandler(() => {
    console.info("Authenticated");
});
api.setMessageHandler((message) => {
    const messageElement = document.importNode(messageTemplate.content, true);
    const usernameElement = messageElement.querySelector(".message-username");
    const contentElement = messageElement.querySelector(".message-content");
    usernameElement.textContent = message.username;
    contentElement.textContent = message.content;
    messageContainer.appendChild(messageElement);
});

api.connect();

function sendMessage() {
    if (messageInput.value.length === 0) {
        return;
    }
    api.sendMessage(messageInput.value);
    messageInput.value = "";
}

messageInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        sendMessage();
    }
});
document.getElementById("send-button").addEventListener("click", sendMessage);