import * as api from "./api.js";
import * as cookies from "./cookies.js";

const messageInput = document.getElementById("message-input");
const messageTemplate = document.getElementById("message-template");
const messageContainer = document.getElementById("message-container");

let sendButton = document.getElementById("send-button");

let addFriendButton = document.getElementById("add-friend");
let addFriendMenu = document.getElementById("add-friend-menu");
let addFriendURL = document.getElementById("add-friend-url");
let addFriendClose = document.getElementById("add-friend-close");

let acceptFriendMenu = document.getElementById("accept-friend-menu");
let acceptFriendText = document.getElementById("accept-friend-text");
let acceptFriendAccept = document.getElementById("accept-friend-accept");
let acceptFriendDecline = document.getElementById("accept-friend-decline");
let acceptFriendClose = document.getElementById("accept-friend-close");
let openingAcceptFriendMenu = false;
let acceptFriendUserID;

let addFriendCopyURL;

let userID;

api.setConnectHandler(() => {
    const userCookie = cookies.getCookie("user");
    if (!userCookie) {
        return;
    }
    const sessionCookie = cookies.getCookie("sessionID");
    if (!sessionCookie) {
        return;
    }
    api.authenticate(atob(userCookie), atob(sessionCookie));
});
api.setAuthHandler(() => {
    const addFriendCookie = cookies.getCookie("add-friend");
    if (addFriendCookie) {
        openAcceptFriendMenu(addFriendCookie);
    }
});
api.setMessageHandler((message) => {
    const messageElement = document.importNode(messageTemplate.content, true);
    const usernameElement = messageElement.querySelector(".message-username");
    const contentElement = messageElement.querySelector(".message-content");
    usernameElement.textContent = message.username;
    contentElement.textContent = message.content;
    messageContainer.appendChild(messageElement);
});
api.setUsernameRequestHandler((username, userID) => {
    if (openingAcceptFriendMenu && btoa(userID) === acceptFriendUserID) {
        acceptFriendText.innerHTML = "Accept friend request from " + username + "?";
        acceptFriendMenu.showModal();
    }
});

api.connect();

function sendMessage() {
    if (messageInput.value.length === 0) {
        return;
    }
    api.sendMessage(messageInput.value);
    messageInput.value = "";
}

function openAddFriendMenu() {
    addFriendURL.innerHTML = window.location.origin + "/friend/" + userID + "<button id=\"copy-url-button\" class=\"copy-button\">Copy</button>";
    addFriendMenu.showModal();
    addFriendCopyURL = document.getElementById("copy-url-button");
    addFriendCopyURL.addEventListener("click", () => {
        navigator.clipboard.writeText(addFriendURL.innerText.substring(0, addFriendURL.innerText.length - 4)).catch(() => {
            console.error("Failed to copy URL to clipboard");
        });
    });
}
function closeAddFriendMenu() {
    addFriendMenu.close();
}

function openAcceptFriendMenu(id) {
    if (userID === id) {
        cookies.deleteCookie("add-friend");
        return;
    }
    openingAcceptFriendMenu = true;
    acceptFriendUserID = id;
    api.requestUsername(id);
}
function closeAcceptFriendMenu() {
    acceptFriendMenu.close();
}
function acceptFriend() {
    api.acceptFriendRequest(acceptFriendUserID);
    closeAcceptFriendMenu();
    cookies.deleteCookie("add-friend");
}

function declineFriend() {
    closeAcceptFriendMenu();
    cookies.deleteCookie("add-friend");
}

messageInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        sendMessage();
    }
});

sendButton.addEventListener("click", sendMessage);

addFriendButton.addEventListener("click", openAddFriendMenu);
addFriendClose.addEventListener("click", closeAddFriendMenu);

acceptFriendClose.addEventListener("click", closeAcceptFriendMenu);
acceptFriendAccept.addEventListener("click", acceptFriend);
acceptFriendDecline.addEventListener("click", declineFriend);