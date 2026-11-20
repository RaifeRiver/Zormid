import * as api from "./api.js";
import * as base64 from "./base64.js";
import * as cookies from "./cookies.js";
import * as emojis from "./emojis.js";
import * as markdown from "./markdown.js";

let appContainer = document.getElementById("app-container");
let loadingScreen = document.getElementById("loading-screen");

let channelContainer = document.getElementById("channel-container");
let messageInput = document.getElementById("message-input");
let messageTemplate = document.getElementById("message-template");
let continuationMessageTemplate = document.getElementById("continuation-message-template");
let messageContainer = document.getElementById("message-container");

let sendButton = document.getElementById("send-button");

let addFriendMenu = document.getElementById("add-friend-menu");
let addFriendURL = document.getElementById("add-friend-url");
let addFriendClose = document.getElementById("add-friend-close");
let addFriendCopyError = document.getElementById("add-friend-copy-error");
let addFriendCopyURL;

let acceptFriendMenu = document.getElementById("accept-friend-menu");
let acceptFriendText = document.getElementById("accept-friend-text");
let acceptFriendAccept = document.getElementById("accept-friend-accept");
let acceptFriendDecline = document.getElementById("accept-friend-decline");
let acceptFriendClose = document.getElementById("accept-friend-close");
let openingAcceptFriendMenu = false;
let acceptFriendUserID;

let channelList = document.getElementById("channel-list");
let addChannelTemplate = document.getElementById("add-channel-template");
let channelTemplate = document.getElementById("channel-template");

let channelName = document.getElementById("channel-name");

let friendListContainer = document.getElementById("friend-list-container");
let friendContainer = document.getElementById("friend-container");
let addFriendTemplate = document.getElementById("add-friend-template");
let friendTemplate = document.getElementById("friend-template");

let addChannelMenu = document.getElementById("add-channel-menu");
let addChannelClose = document.getElementById("add-channel-close");
let addChannelFriends = document.getElementById("add-channel-friends");
let addChannelFriendTemplate = document.getElementById("add-channel-friend-template");
let addChannelAdd = document.getElementById("add-channel-add");
let addChannelName = document.getElementById("add-channel-name");

let openFriendsListButton = document.getElementById("open-friends-list");

let userNameDisplay = document.getElementById("user-name");
let userSettingsButton = document.getElementById("user-settings-button");

let channelSettingsContainer = document.getElementById("channel-settings-container");
let channelUserListButton = document.getElementById("channel-user-list-button");

let userListContainer = document.getElementById("user-list-container");
let userListUserTemplate = document.getElementById("user-list-user-template");
let userListManageTemplate = document.getElementById("user-list-manage-template");

let userListManageUsersMenu = document.getElementById("user-list-manage-users-menu");
let userListManageUsersClose = document.getElementById("user-list-manage-users-close");
let userListManageUsersUsers = document.getElementById("user-list-manage-users-users");
let userListManageUsersConfirm = document.getElementById("user-list-manage-users-confirm");

let emojiList = document.getElementById("emoji-list");
let emojiListList = document.getElementById("emoji-list-list");
let emojiListInput = document.getElementById("emoji-list-input");

let userSettingsMenu = document.getElementById("user-settings-menu");
let userSettingsProfileTab = document.getElementById("user-settings-profile-tab");
let userSettingsProfile = document.getElementById("user-settings-profile");
let userSettingsProfileUsername = document.getElementById("user-settings-profile-username");
let userSettingsProfileLogout = document.getElementById("user-settings-profile-logout");
let userSettingsAppearanceTab = document.getElementById("user-settings-appearance-tab");
let userSettingsAppearance = document.getElementById("user-settings-appearance");
let userSettingsAppearanceTheme = document.getElementById("user-settings-appearance-theme");
let userSettingsClose = document.getElementById("user-settings-close");

let userID;
let currentChannelID;
let channelMessageCount;
let friends = [];
let waitingForChannelMessages = false;
let connectionAttempts = 0;
let emojiEntries = [];
let currentUserSettingsContent = userSettingsProfile;

let messageScrollObserver;

let currentTheme = localStorage.getItem("theme") || "sync";
document.documentElement.setAttribute("data-theme", currentTheme);
userSettingsAppearanceTheme.value = currentTheme;

api.setConnectHandler(() => {
    const userCookie = cookies.getCookie("user");
    if (!userCookie) {
        return;
    }
    const sessionCookie = cookies.getCookie("sessionID");
    if (!sessionCookie) {
        return;
    }
    userID = atob(userCookie);
    api.authenticate(userID, atob(sessionCookie));
});
api.setAuthHandler((username) => {
    userNameDisplay.textContent = username;
    userSettingsProfileUsername.value = username;

    const addFriendCookie = cookies.getCookie("add-friend");
    if (addFriendCookie) {
        openAcceptFriendMenu(addFriendCookie.replaceAll('-', '+').replaceAll('_', '/'));
    }
    api.requestChannelList(userID);
    api.requestFriendList();

    loadingScreen.style.display = "none";
    appContainer.style.display = "flex";

    window.addEventListener("popstate", (event) => setState(event.state));
    if (history.state) {
        setState(history.state);
    }
});
api.setFriendListRequestHandler((f) => {
    friends = f;
    friendContainer.replaceChildren();
    let addFriendElement = document.importNode(addFriendTemplate.content, true);
    friendContainer.appendChild(addFriendElement);
    document.getElementById("add-friend").addEventListener("click", openAddFriendMenu);
    for (let friend of friends) {
        let friendElement = document.importNode(friendTemplate.content, true);
        let friendButton = friendElement.firstElementChild;
        friendButton.textContent = friend.username;
        friendContainer.appendChild(friendElement);
    }
});
api.setUsernameRequestHandler((username, userID) => {
    if (openingAcceptFriendMenu && btoa(userID) === acceptFriendUserID) {
        acceptFriendText.innerHTML = "Accept friend request from " + username + "?";
        acceptFriendMenu.showModal();
    }
});
api.setChannelListRequestHandler((channels, id) => {
    if (id !== userID) {
        return;
    }
    channelList.replaceChildren();
    let addChannelElement = document.importNode(addChannelTemplate.content, true);
    channelList.appendChild(addChannelElement);
    document.getElementById("add-channel").addEventListener("click", openAddChannelMenu);
    for (let channel of channels) {
        let channelElement = document.importNode(channelTemplate.content, true);
        let channelButton = channelElement.firstElementChild;
        channelButton.textContent = channel.channelName;
        channelButton.addEventListener("click", () => changeChannel(channel.channelID));
        channelList.appendChild(channelElement);
    }
});
api.setMessageHandler((message) => {
    if (message.channelID !== currentChannelID && message.userID !== userID) {
        return;
    }
    insertMessage(message.index, message.username, btoa(message.userID), emojis.toEmojis(message.content));
});
api.setChannelInfoHandler((info) => {
    if (info.channelID !== currentChannelID) {
        return;
    }
    channelName.textContent = info.name;
    channelMessageCount = info.messageCount;
    channelSettingsContainer.style.display = "flex";
    messageContainer.replaceChildren();
    if (channelMessageCount > 0) {
        api.requestChannelMessages(currentChannelID, Math.max(channelMessageCount - 50, 0));
        waitingForChannelMessages = true;
    }
    userListContainer.replaceChildren();
    let manageUsersElement = userListManageTemplate.content.cloneNode(true);
    manageUsersElement.querySelector("#user-list-manage-users").addEventListener("click", manageChannelUsers);
    userListContainer.appendChild(manageUsersElement);
    userListManageUsersUsers.replaceChildren();
    for (let user of info.users) {
        let userElement = userListUserTemplate.content.cloneNode(true);
        userElement.querySelector(".user-list-username").textContent = user.username;
        userListContainer.appendChild(userElement);
        let manageElement = addChannelFriendTemplate.content.cloneNode(true);
        manageElement.querySelector(".add-channel-friend-name").textContent = user.username;
        let manageButton = manageElement.querySelector(".add-channel-friend-button");
        manageButton.textContent = "Remove";
        manageButton.addEventListener("click", (e) => {
            e.target.textContent = e.target.textContent === "Add" ? "Remove" : "Add";
        });
        manageElement.firstElementChild.dataset.id = btoa(user.userID);
        userListManageUsersUsers.appendChild(manageElement);
    }
    for (let user of friends) {
        if (info.users.map(user => user.userID).includes(user.userID)) {
            continue;
        }
        let manageElement = addChannelFriendTemplate.content.cloneNode(true);
        manageElement.querySelector(".add-channel-friend-name").textContent = user.username;
        let manageButton = manageElement.querySelector(".add-channel-friend-button");
        manageButton.textContent = "Add";
        manageButton.addEventListener("click", (e) => {
            e.target.textContent = e.target.textContent === "Add" ? "Remove" : "Add";
        });
        manageElement.firstElementChild.dataset.id = btoa(user.userID);
        userListManageUsersUsers.appendChild(manageElement);
    }
});
api.setChannelMessagesHandler((messageInfo) => {
    if (messageInfo.channelID !== currentChannelID) {
        return;
    }
    for (let i = 0; i < messageInfo.messages.length; i++) {
        insertMessage(messageInfo.firstMessage + i, messageInfo.messages[i].username, btoa(messageInfo.messages[i].userID), emojis.toEmojis(messageInfo.messages[i].content));
    }
    if (waitingForChannelMessages) {
        messageContainer.scrollTop = messageContainer.scrollHeight;
        waitingForChannelMessages = false;
    }
});
api.setConnectionLostHandler(() => {
    appContainer.style.display = "none";
    loadingScreen.querySelector("#loading-text").innerHTML = "Connection lost. Reconnecting...";
    loadingScreen.style.display = "flex";
    connectionAttempts = 1;
    api.connect();
});

api.setTimeoutHandler(() => {
    if (connectionAttempts === 5) {
        loadingScreen.querySelector("#loading-text").innerHTML = "Failed to connect. Please try again later.";
        return;
    }
    loadingScreen.querySelector("#loading-text").innerHTML = "Error connecting. Retrying...";
    setTimeout(() => {
        connectionAttempts++;
        api.connect();
    }, 5000);
});

connectionAttempts++;
api.connect();

emojis.setLoadHandler(() => {
    emojiEntries = Object.entries(emojis.emojis);
    let emojiHTML = "";
    for (let i = 0; i < emojiEntries.length; i++) {
        emojiHTML += `<li id="emoji-list-${i}"><button>${emojiEntries[i][1]}</button></li>`;
    }
    emojiListList.innerHTML = emojiHTML;
    for (let i = 0; i < emojiEntries.length; i++) {
        document.getElementById("emoji-list-" + i).addEventListener("click", () => {
            messageInput.value += emojiEntries[i][1];
        });
    }
    searchEmojiList();
});
emojis.loadEmojis();

function sendMessage() {
    if (messageInput.value.length === 0) {
        return;
    }
    if (currentChannelID) {
        api.sendMessage(emojis.toPlainText(messageInput.value), currentChannelID);
    }
    messageInput.value = "";
    messageContainer.scrollTop = messageContainer.scrollHeight;
}

function tryDeprecatedFriendURLCopy() {
    if (document.queryCommandSupported("copy")) {
        let textArea = document.createElement("textarea");
        textArea.value = addFriendURL.innerText.substring(0, addFriendURL.innerText.length - 4);
        textArea.style.position = "fixed";
        textArea.style.left = "-10000px";
        addFriendMenu.appendChild(textArea);
        textArea.focus();
        textArea.select();
        if (document.execCommand("copy")) {
            addFriendMenu.removeChild(textArea);
            return;
        }
        addFriendMenu.removeChild(textArea);
    }
    addFriendCopyError.textContent = "Failed to copy URL to clipboard";
    addFriendCopyError.style.display = "inline";
    setTimeout(() => {
        addFriendCopyError.style.display = "none";
    }, 2000);
}

function openAddFriendMenu() {
    addFriendURL.innerHTML = window.location.origin + "/friend/" +btoa(userID).replaceAll('+', '-').replaceAll('/', '_') + "<button id=\"copy-url-button\" class=\"copy-button\">Copy</button>";
    addFriendMenu.showModal();
    addFriendCopyURL = document.getElementById("copy-url-button");
    addFriendCopyURL.addEventListener("click", () => {
        if (navigator.clipboard) {
            navigator.clipboard.writeText(addFriendURL.innerText.substring(0, addFriendURL.innerText.length - 4)).catch(() => {
                tryDeprecatedFriendURLCopy();
            });
        }
        else {
            tryDeprecatedFriendURLCopy();
        }
    });
}

function closeAddFriendMenu() {
    addFriendMenu.close();
}

function openAcceptFriendMenu(id) {
    if (userID === atob(id)) {
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
    api.acceptFriendRequest(atob(acceptFriendUserID));
    closeAcceptFriendMenu();
    cookies.deleteCookie("add-friend");
}

function declineFriend() {
    closeAcceptFriendMenu();
    cookies.deleteCookie("add-friend");
}

function changeChannel(channelID) {
    if (channelID === currentChannelID) {
        return;
    }
    history.pushState({channel: channelID}, "", "/app/channel/" + base64.encodeURL(channelID));
    if (!currentChannelID) {
        friendListContainer.style.display = "none";
        channelContainer.style.display = "flex";
    }
    currentChannelID = channelID;
    messageContainer.replaceChildren();
    api.requestChannelInfo(channelID);
}

function openAddChannelMenu() {
    addChannelFriends.replaceChildren();
    for (let friend of friends) {
        let friendElement = document.importNode(addChannelFriendTemplate.content, true);
        friendElement.querySelector(".add-channel-friend-name").textContent = friend.username;
        friendElement.querySelector(".add-channel-friend-button").addEventListener("click", (e) => {
            e.target.textContent = e.target.textContent === "Add" ? "Remove" : "Add";
        });
        addChannelFriends.appendChild(friendElement);
    }
    addChannelMenu.showModal();
}

function closeAddChannelMenu() {
    addChannelMenu.close();
}

function addChannelFromMenu() {
    let name = addChannelName.value;
    if (name === "") {
        addChannelName.placeholder = "Name is required";
        return;
    }
    let selectedFriends = [];
    selectedFriends.push(userID);
    for (let friend of addChannelFriends.children) {
        let selected = friend.querySelector(".add-channel-friend-button").textContent === "Remove";
        if (selected) {
            let username = friend.querySelector(".add-channel-friend-name").textContent;
            for (let f of friends) {
                if (f.username === username) {
                    selectedFriends.push(f.userID);
                }
            }
        }
    }
    api.createChannel(name, selectedFriends);
    addChannelMenu.close();
}

function openFriendsList() {
    if (!currentChannelID) {
        return;
    }
    history.pushState({}, "", "/app/");
    currentChannelID = undefined;
    channelContainer.style.display = "none";
    channelName.textContent = "Friends";
    channelSettingsContainer.style.display = "none";
    friendListContainer.style.display = "block";
}

function processMessageContents(message) {
    return markdown.convertToHTML(message);
}

function insertMessage(index, from, fromID, content) {
    let shouldScroll = messageContainer.scrollTop + messageContainer.clientHeight >= messageContainer.scrollHeight - 2;
    let previousMessageElement;
    let previousIndex;
    let nextMessageElement;
    for (let i = 0; i < messageContainer.children.length; i++) {
        let child = messageContainer.children[i];
        let childIndex = parseInt(child.dataset.index);
        if (childIndex === index) {
            return;
        }
        if (childIndex > index - 1) {
            if (child.dataset.username === from) {
                nextMessageElement = child;
            }
            break;
        }
        if (child.dataset.username === from) {
            previousMessageElement = child;
        }
        else {
            previousMessageElement = null;
        }
        previousIndex = i;
    }
    if (previousMessageElement) {
        let messageElement = document.importNode(continuationMessageTemplate.content, true);
        let contentElement = messageElement.querySelector(".message-content");
        contentElement.innerHTML = processMessageContents(content);
        messageElement.firstElementChild.dataset.username = from;
        messageElement.firstElementChild.dataset.index = index;
        messageContainer.insertBefore(messageElement, messageContainer.children[previousIndex + 1]);
    }
    else {
        let messageElement = document.importNode(messageTemplate.content, true);
        let usernameElement = messageElement.querySelector(".message-username");
        let contentElement = messageElement.querySelector(".message-content");
        usernameElement.textContent = from;
        contentElement.innerHTML = processMessageContents(content);
        messageElement.firstElementChild.dataset.username = from;
        messageElement.firstElementChild.dataset.index = index;
        if (previousIndex !== undefined) {
            messageContainer.insertBefore(messageElement, messageContainer.children[previousIndex + 1]);
        }
        else {
            messageContainer.prepend(messageElement);
        }
    }
    if (nextMessageElement) {
        let modifiedNextElement = document.importNode(continuationMessageTemplate.content, true);
        let contentElement = modifiedNextElement.querySelector(".message-content");
        contentElement.innerHTML = nextMessageElement.querySelector(".message-content").innerHTML;
        modifiedNextElement.firstElementChild.dataset.username = nextMessageElement.dataset.username;
        modifiedNextElement.firstElementChild.dataset.index = nextMessageElement.dataset.index;
        messageContainer.replaceChild(modifiedNextElement, nextMessageElement);
    }
    if (shouldScroll) {
        messageContainer.scrollTop = messageContainer.scrollHeight;
    }
    setupMessageScrollObserver();
}

function setupMessageScrollObserver() {
    if (messageScrollObserver) {
        messageScrollObserver.disconnect();
    }
    messageScrollObserver = new IntersectionObserver((entries, _) => {
        for (let entry of entries) {
            if (!entry.isIntersecting) {
                continue;
            }
            let currentFirstMessage = parseInt(entry.target.dataset.index);
            api.requestChannelMessages(currentChannelID, currentFirstMessage <= 50 ? 0 : currentFirstMessage - 50);
        }
    }, {
        threshold: 1
    });
    messageScrollObserver.observe(messageContainer.firstElementChild);
}

function toggleUserList() {
    userListContainer.style.display = userListContainer.style.display === "none" ? "flex" : "none";
}

function documentClickHandler(event) {
    if (!emojiList.contains(event.target)) {
        emojiList.open = false;
    }
}

function searchEmojiList() {
    let search = emojiListInput.value.toLowerCase().split(" ");
    if (search.length === 0) {
        for (let emoji of emojiListList.children) {
            emoji.style.display = "inline-block";
        }
        return;
    }
    for (let emoji of emojiListList.children) {
        let name = emojiEntries[parseInt(emoji.id.substring(11))][0];
        let contains = true;
        for (let word of search) {
            if (!name.includes(word)) {
                contains = false;
                break;
            }
        }
        emoji.style.display = contains ? "inline-block" : "none";
    }
}

function openUserSettingsMenu() {
    userSettingsMenu.showModal();
}

function openUserSettingsTab(tab) {
    currentUserSettingsContent.style.display = "none";
    currentUserSettingsContent = tab;
    currentUserSettingsContent.style.display = "flex";
}

function closeUserSettingsMenu() {
    userSettingsMenu.close();
}

function changeUsernameFromSettings() {
    if (userSettingsProfileUsername.value.length > 0) {
        api.changeUsername(userSettingsProfileUsername.value);
        userNameDisplay.textContent = userSettingsProfileUsername.value;
        userSettingsProfileUsername.value = "";
    }
}

function changeThemeFromSettings() {
    let theme = userSettingsAppearanceTheme.value;
    document.documentElement.setAttribute("data-theme", theme);
    localStorage.setItem("theme", theme);
}

function manageChannelUsers() {
    userListManageUsersMenu.showModal();
}

function closeManageChannelUsersMenu() {
    userListManageUsersMenu.close();
}

function changeChannelUsers() {
    let users = []
    for (let user of userListManageUsersUsers.children) {
        if (user.querySelector(".add-channel-friend-button").textContent === "Remove") {
            users.push(atob(user.dataset.id));
        }
    }
    api.changeChannelUsers(currentChannelID, users);
    userListManageUsersMenu.close();
}

function logout() {
    cookies.deleteCookie("user");
    cookies.deleteCookie("sessionID");
    window.location.href = "/auth";
}

function setState(state) {
    if (state.channel) {
        changeChannel(state.channel);
    }
    else {
        openFriendsList();
    }
}

messageInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        sendMessage();
    }
});

sendButton.addEventListener("click", sendMessage);

addFriendClose.addEventListener("click", closeAddFriendMenu);

acceptFriendClose.addEventListener("click", closeAcceptFriendMenu);
acceptFriendAccept.addEventListener("click", acceptFriend);
acceptFriendDecline.addEventListener("click", declineFriend);

addChannelClose.addEventListener("click", closeAddChannelMenu);
addChannelAdd.addEventListener("click", addChannelFromMenu);

openFriendsListButton.addEventListener("click", openFriendsList);

channelUserListButton.addEventListener("click", toggleUserList);

document.addEventListener("click", documentClickHandler);

emojiListInput.addEventListener("input", searchEmojiList)

userSettingsButton.addEventListener("click", openUserSettingsMenu);
userSettingsProfileTab.addEventListener("click", () => openUserSettingsTab(userSettingsProfile));
userSettingsProfileUsername.addEventListener("change", changeUsernameFromSettings);
userSettingsProfileLogout.addEventListener("click", logout);
userSettingsAppearanceTab.addEventListener("click", () => openUserSettingsTab(userSettingsAppearance));
userSettingsAppearanceTheme.addEventListener("change", changeThemeFromSettings);
userSettingsClose.addEventListener("click", closeUserSettingsMenu);

userListManageUsersClose.addEventListener("click", closeManageChannelUsersMenu);
userListManageUsersConfirm.addEventListener("click", changeChannelUsers);