import * as api from "./api.js";
import * as base64 from "./base64.js";
import * as cookies from "./cookies.js";
import * as emojis from "./emojis.js";
import * as markdown from "./markdown.js";

let appContainer = document.getElementById("app-container");
let loadingScreen = document.getElementById("loading-screen");
let sidePanel = document.getElementById("side-panel");
let mainArea = document.getElementById("main-area");

let textChannelContainer = document.getElementById("text-channel-container");
let messageInput = document.getElementById("message-input");
let messageTemplate = document.getElementById("message-template");
let continuationMessageTemplate = document.getElementById("continuation-message-template");
let messageContainer = document.getElementById("message-container");
let typingIndicator = document.getElementById("typing-indicator");

let voiceChannelContainer = document.getElementById("voice-channel-container");

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
let addChannelType = document.getElementById("add-channel-type");
let addChannelFriends = document.getElementById("add-channel-friends");
let addChannelFriendTemplate = document.getElementById("add-channel-friend-template");
let addChannelAdd = document.getElementById("add-channel-add");
let addChannelName = document.getElementById("add-channel-name");

let openFriendsListButton = document.getElementById("open-friends-list");

let userInfoContainer = document.getElementById("user-info");
let userAvatar = document.getElementById("user-avatar");
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

let insertInput = document.getElementById("insert-input");
let uploadFileButton = document.getElementById("upload-file-button");
let uploadFileInput = document.getElementById("file-upload-input");

let fileDownloadLink = document.getElementById("file-download-link");

let emojiList = document.getElementById("emoji-list");
let emojiListList = document.getElementById("emoji-list-list");
let emojiListInput = document.getElementById("emoji-list-input");

let userSettingsMenu = document.getElementById("user-settings-menu");
let userSettingsProfileTab = document.getElementById("user-settings-profile-tab");
let userSettingsProfile = document.getElementById("user-settings-profile");
let userSettingsProfileChangeAvatar = document.getElementById("user-settings-profile-change-avatar");
let userSettingsProfileAvatar = document.getElementById("user-settings-profile-avatar");
let userSettingsProfileUsername = document.getElementById("user-settings-profile-username");
let userSettingsProfileLogout = document.getElementById("user-settings-profile-logout");
let userSettingsSessionsTab = document.getElementById("user-settings-sessions-tab");
let userSettingsSessions = document.getElementById("user-settings-sessions");
let userSettingsSessionsList = document.getElementById("user-settings-sessions-list");
let userSettingsSessionsSessionTemplate = document.getElementById("user-settings-sessions-session-template");
let userSettingsAppearanceTab = document.getElementById("user-settings-appearance-tab");
let userSettingsAppearance = document.getElementById("user-settings-appearance");
let userSettingsAppearanceTheme = document.getElementById("user-settings-appearance-theme");
let userSettingsClose = document.getElementById("user-settings-close");

let backButton = document.getElementById("back-button");

let portraitMedia = window.matchMedia("(orientation: portrait)");

let userID;
let currentChannelID;
let currentChannelType;
let channelMessageCount;
let channelTypingUsers = [];
let friends = [];
let waitingForChannelMessages = false;
let connectionAttempts = 0;
let emojiEntries = [];
let currentUserSettingsContent = userSettingsProfile;
let uploadType;
let users = {};
let connected = false;
let typing = false;
let stateSet = false;
let callStream;
let callConnection;
let pendingICECandidates = [];

let messageScrollObserver;

let currentTheme = localStorage.getItem("theme") || "sync";
document.documentElement.setAttribute("data-theme", currentTheme);
userSettingsAppearanceTheme.value = currentTheme;

api.setConnectHandler(() => {
    if (connected) {
        window.location.reload();
        return;
    }
    connected = true;
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
api.setAuthHandler((info) => {
    userNameDisplay.textContent = info.username;
    userSettingsProfileUsername.value = info.username;

    let avatar = info.avatar === "DEFAULT" ? "/res/images/default-avatar.png" : info.avatar;
    userAvatar.src = avatar;
    userSettingsProfileAvatar.src = avatar;

    setUserInfo(userID, info);
    addUserElement(userID, userInfoContainer);

    const addFriendCookie = cookies.getCookie("add-friend");
    if (addFriendCookie) {
        try {
            openAcceptFriendMenu(base64.decodeURL(addFriendCookie));
        }
        catch (e) {}
    }
    api.requestChannelList(userID);
    api.requestFriendList();

    loadingScreen.style.display = "none";
    appContainer.style.display = "flex";

    document.title = "Zormid - Friends";

    window.addEventListener("popstate", (event) => setState(event.state));
});
api.setFriendListRequestHandler((f) => {
    friends = f;
    friendContainer.replaceChildren();
    let addFriendElement = document.importNode(addFriendTemplate.content, true);
    friendContainer.appendChild(addFriendElement);
    document.getElementById("add-friend").addEventListener("click", openAddFriendMenu);
    for (let friend of friends) {
        let friendElement = document.importNode(friendTemplate.content, true).firstElementChild;
        friendElement.querySelector(".friend-avatar").src = friend.avatar;
        friendElement.querySelector(".friend-username").textContent = friend.username;
        friendContainer.appendChild(friendElement);
        addUserElement(friend.userID, friendElement);
        setUserInfo(friend.userID, friend);
    }
});
api.setUsernameRequestHandler((username, userID) => {
    if (openingAcceptFriendMenu && userID === acceptFriendUserID) {
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
        addChannel(channel);
    }
    if (!stateSet) {
        stateSet = true;
        if (history.state) {
            setState(history.state);
        }
    }
});
api.setMessageHandler((message) => {
    if (message.channelID !== currentChannelID && message.userID !== userID) {
        return;
    }
    insertMessage(message.index, message.userID, emojis.toEmojis(message.content), message.timestamp);
});
api.setChannelInfoHandler((info) => {
    if (info.channelID !== currentChannelID) {
        return;
    }
    document.title = `Zormid - ${info.channelName}`;
    channelName.textContent = info.channelType === api.CHANNEL_TYPE_VOICE ? `📞 ${info.channelName}`: info.channelName;
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
        userElement.querySelector(".user-list-avatar").src = user.avatar;
        userElement.querySelector(".user-list-username").textContent = user.username;
        addUserElement(user.userID, userElement.firstElementChild);
        userListContainer.appendChild(userElement);
        let manageElement = addChannelFriendTemplate.content.cloneNode(true);
        manageElement.querySelector(".add-channel-friend-name").textContent = user.username;
        let manageButton = manageElement.querySelector(".add-channel-friend-button");
        manageButton.textContent = "Remove";
        manageButton.addEventListener("click", (e) => {
            e.target.textContent = e.target.textContent === "Add" ? "Remove" : "Add";
        });
        manageElement.firstElementChild.dataset.id = btoa(user.userID);
        addUserElement(user.userID, manageElement.firstElementChild);
        userListManageUsersUsers.appendChild(manageElement);
        setUserInfo(user.userID, user);
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
        insertMessage(messageInfo.firstMessage + i, messageInfo.messages[i].userID, emojis.toEmojis(messageInfo.messages[i].content), messageInfo.messages[i].timestamp);
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

api.setUploadFailedHandler((id, message) => {
    messageInput.value = messageInput.value.replace(new RegExp(`!\\[.*?]\\(${window.location.origin}/upload/${id}\\)`, "g"), "");
    window.requestAnimationFrame(() => alert(`Upload failed: ${message}`));
});

api.setSessionsRequestHandler((sessions) => {
    userSettingsSessionsList.replaceChildren();
    for (let i = 0; i < sessions.length; i++) {
        let session = userSettingsSessionsSessionTemplate.content.cloneNode(true);
        session.querySelector(".user-settings-sessions-session-user-agent").innerHTML = sessions[i];
        session.querySelector(".user-settings-sessions-session-logout").addEventListener("click", () => {
            let userAgent = sessions[i];
            api.logoutSession(userAgent);
            if (userAgent === navigator.userAgent) {
                window.location.href = "/auth";
            }
        });
        userSettingsSessionsList.appendChild(session);
    }
});

api.setUserInfoHandler((info) => setUserInfo(info.userID, info));

api.setChannelAddHandler(addChannel);

api.setChannelRemoveHandler((channelID) => {
    if (channelID === currentChannelID) {
        openFriendsList();
    }
    channelList.querySelector(`[data-id="${base64.encodeURL(channelID)}"]`).remove();
});

api.setUserTypingChangeHandler((info) => {
    if (info.channelID !== currentChannelID) {
        return;
    }
    if (info.typing) {
        channelTypingUsers.push(info.userID);
    }
    else {
        channelTypingUsers.splice(channelTypingUsers.indexOf(info.userID), 1);
    }
    if (channelTypingUsers.length === 0) {
        typingIndicator.textContent = "";
        return;
    }
    let typingString = "";
    let lastUsername;
    for (let userID of channelTypingUsers) {
        if (lastUsername) {
            typingString += ", " + lastUsername;
        }
        lastUsername = users[userID]?.username || "Unknown";
    }
    if (lastUsername) {
        typingString += typingString.length === 0 ? lastUsername : ` and ${lastUsername}`;
    }
    typingIndicator.textContent = `${typingString} ${channelTypingUsers.length === 1 ? "is" : "are"} typing...`;
});

api.setCallJoinedHandler(async (joinedID) => {
    callConnection = new RTCPeerConnection({iceServers: [{urls: "stun:stun.cloudflare.com:3478"}, {urls: "turn:free.expressturn.com:3478", username: "000000002111055202", credential: "+RFLZyMU/RHslWGpjuF6JyT4xt8="}]});
    callConnection.addEventListener("track", (event) => {
        let audioElement = document.createElement("audio");
        audioElement.autoplay = true;
        audioElement.srcObject = event.streams[0];
        audioElement.style.display = "none";
        document.body.appendChild(audioElement);
    });
    for (let track of callStream.getTracks()) {
        callConnection.addTrack(track, callStream);
    }
    let offer = await callConnection.createOffer();
    await callConnection.setLocalDescription(offer);
    callConnection.addEventListener("icecandidate", (event) => {
        if (event.candidate) {
            api.sendCallCandidate(joinedID, event.candidate);
        }
    });
    api.sendCallOffer(joinedID, offer, false);
});

api.setCallOfferHandler(async (offer, response, fromID) => {
    if (response) {
        await callConnection.setRemoteDescription(offer);
    }
    else {
        callConnection = new RTCPeerConnection({iceServers: [{urls: "stun:stun.cloudflare.com:3478"}, {urls: "turn:free.expressturn.com:3478", username: "000000002111055202", credential: "+RFLZyMU/RHslWGpjuF6JyT4xt8="}]});
        callConnection.addEventListener("icecandidate", (event) => {
            if (event.candidate) {
                api.sendCallCandidate(fromID, event.candidate);
            }
        });
        callConnection.addEventListener("track", (event) => {
            let audioElement = document.createElement("audio");
            audioElement.autoplay = true;
            audioElement.srcObject = event.streams[0];
            document.body.appendChild(audioElement);
        });
        for (let track of callStream.getTracks()) {
            callConnection.addTrack(track, callStream);
        }
        await callConnection.setRemoteDescription(offer);
        let localOffer = await callConnection.createAnswer();
        await callConnection.setLocalDescription(localOffer);
        api.sendCallOffer(fromID, localOffer, true);
    }
    for (let candidate of pendingICECandidates) {
        await callConnection.addIceCandidate(candidate);
    }
    pendingICECandidates.length = 0;
});

api.setCallCandidateHandler(async (candidate) => {
    if (callConnection.remoteDescription) {
        await callConnection.addIceCandidate(candidate);
    }
    else {
        pendingICECandidates.push(candidate);
    }
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
    messageInput.dispatchEvent(new Event("input"));
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
    addFriendURL.innerHTML = `<span>${window.location.origin}/friend/${base64.encodeURL(userID)}</span><button id="copy-url-button" class="copy-button">Copy</button>`;
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
    if (userID === id) {
        cookies.deleteCookie("add-friend");
        return;
    }
    openingAcceptFriendMenu = true;
    acceptFriendUserID = id;
    api.requestUsername(btoa(id));
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

function changeChannel(channelID) {
    if (channelID === currentChannelID) {
        return;
    }
    let encodedChannelID = base64.encodeURL(channelID);
    history.pushState({channel: channelID}, "", `/app/channel/${encodedChannelID}`);
    let type = parseInt(channelList.querySelector(`[data-id="${encodedChannelID}"]`).dataset.type);
    if (!currentChannelID) {
        friendListContainer.style.display = "none";
        if (portraitMedia.matches) {
            sidePanel.style.display = "none";
            mainArea.style.display = "flex";
        }
    }
    else {
        api.setTyping(false, currentChannelID);
    }
    if (currentChannelType !== type) {
        if (type === api.CHANNEL_TYPE_TEXT) {
            voiceChannelContainer.style.display = "none";
            textChannelContainer.style.display = "flex";
        }
        else if (type === api.CHANNEL_TYPE_VOICE) {
            textChannelContainer.style.display = "none";
            voiceChannelContainer.style.display = "flex";
        }
    }
    channelTypingUsers.length = 0;
    typing = false;
    currentChannelID = channelID;
    if (type === api.CHANNEL_TYPE_TEXT) {
        messageContainer.replaceChildren();
    }
    else if (type === api.CHANNEL_TYPE_VOICE) {
        navigator.mediaDevices.getUserMedia({
            audio: {
                noiseSuppression: true,
                echoCancellation: true,
                autoGainControl: true
            }
        }).then((stream) => {
            callStream = stream;
            api.joinCall(channelID);
        });
    }
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
    api.createChannel(parseInt(addChannelType.value), name, selectedFriends);
    addChannelMenu.close();
}

function openFriendsList() {
    if (!currentChannelID) {
        if (portraitMedia.matches) {
            sidePanel.style.display = "none";
            mainArea.style.display = "flex";
        }
        return;
    }
    history.pushState({}, "", "/app/");
    document.title = "Zormid - Friends";
    typing = false;
    api.setTyping(false, currentChannelID);
    currentChannelID = undefined;
    currentChannelType = undefined;
    textChannelContainer.style.display = "none";
    voiceChannelContainer.style.display = "none";
    channelName.textContent = "Friends";
    channelSettingsContainer.style.display = "none";
    userListContainer.style.display = "none";
    friendListContainer.style.display = "block";
    if (portraitMedia.matches) {
        sidePanel.style.display = "none";
        mainArea.style.display = "flex";
    }
}

function processMessageContents(message) {
    return markdown.convertToHTML(message);
}

function insertMessage(index, fromID, content, timestamp) {
    let user = users[fromID];
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
            if (child.dataset.username === user.username) {
                nextMessageElement = child;
            }
            break;
        }
        if (child.dataset.username === user.username) {
            previousMessageElement = child;
        }
        else {
            previousMessageElement = null;
        }
        previousIndex = i;
    }
    if (previousMessageElement && timestamp.getTime() - new Date(previousMessageElement.dataset.timestamp).getTime() < 300000) {
        let messageElement = document.importNode(continuationMessageTemplate.content, true);
        messageElement.querySelector(".message-content").innerHTML = processMessageContents(content);
        messageElement.firstElementChild.dataset.username = user.username;
        messageElement.firstElementChild.dataset.index = index;
        messageElement.firstElementChild.dataset.timestamp = timestamp.toISOString();
        messageContainer.insertBefore(messageElement, messageContainer.children[previousIndex + 1]);
    }
    else {
        let messageElement = document.importNode(messageTemplate.content, true);
        messageElement.querySelector(".message-avatar").src = user.avatar === "DEFAULT" ? "/res/images/default-avatar.png" : user.avatar;
        messageElement.querySelector(".message-username").textContent = user.username;
        messageElement.querySelector(".message-timestamp").textContent = timestamp.toLocaleTimeString([], {hour: "2-digit", minute: "2-digit"});
        messageElement.querySelector(".message-content").innerHTML = processMessageContents(content);
        messageElement.firstElementChild.dataset.username = user.username;
        messageElement.firstElementChild.dataset.index = index;
        messageElement.firstElementChild.dataset.timestamp = timestamp.toISOString();
        let messageChild = messageElement.firstElementChild;
        if (previousIndex !== undefined) {
            messageContainer.insertBefore(messageElement, messageContainer.children[previousIndex + 1]);
        }
        else {
            messageContainer.prepend(messageElement);
        }
        addUserElement(fromID, messageChild);
    }
    if (nextMessageElement && new Date(nextMessageElement.dataset.timestamp).getTime() - timestamp.getTime() < 300000) {
        removeUserElement(fromID, nextMessageElement);
        let modifiedNextElement = document.importNode(continuationMessageTemplate.content, true);
        let contentElement = modifiedNextElement.querySelector(".message-content");
        contentElement.innerHTML = nextMessageElement.querySelector(".message-content").innerHTML;
        modifiedNextElement.firstElementChild.dataset.username = nextMessageElement.dataset.username;
        modifiedNextElement.firstElementChild.dataset.index = nextMessageElement.dataset.index;
        modifiedNextElement.firstElementChild.dataset.timestamp = nextMessageElement.dataset.timestamp;
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
    if (!insertInput.contains(event.target)) {
        insertInput.open = false;
    }
    if (event.target.matches(".file-download")) {
        fileDownloadLink.href = atob(event.target.parentElement.dataset.download);
        fileDownloadLink.download = atob(event.target.parentElement.dataset.name);
        fileDownloadLink.click();
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
    else if (!portraitMedia.matches) {
        openFriendsList();
    }
}

async function uploadMessageFiles(files) {
    for (let file of files) {
        let id = await api.uploadFile(file);
        messageInput.setRangeText(`![${file.name}](${window.location.origin}/upload/${id})`, messageInput.selectionStart, messageInput.selectionEnd, "end");
    }
}

function openUploadDialog() {
    uploadFileInput.click();
    uploadType = "message";
}

function openChangeAvatarDialog() {
    uploadFileInput.click();
    uploadType = "avatar";
}

async function uploadAvatarFile(file) {
    let id = await api.uploadFile(file);
    let url = `${window.location.origin}/upload/${id}`;
    api.setAvatar(url);
    userAvatar.src = url;
    userSettingsProfileAvatar.src = url;
}

function setUserInfo(id, info) {
    let elements = users[id]?.elements || [];
    users[id] = info;
    users[id].elements = elements;
    for (let element of elements) {
        let username = element.querySelector(".user-username");
        if (username) {
            username.textContent = info.username;
        }
        let avatar = element.querySelector(".user-avatar");
        if (avatar) {
            avatar.src = info.avatar === "DEFAULT" ? "/res/images/default-avatar.png" : info.avatar;
        }
        let status = element.querySelector(".user-status");
        if (status) {
            status.style["background-color"] = `var(--theme-${info.status === 0 ? "offline" : "online"}-color)`;
        }
    }
}

function addUserElement(id, element) {
    let elements = users[id]?.elements;
    if (!elements) {
        users[id] = {elements: [element]};
    }
    else {
        elements.push(element);
    }
}

function removeUserElement(id, element) {
    let elements = users[id]?.elements;
    if (!elements) {
        return;
    }
    elements.splice(elements.indexOf(element), 1);
}

function addChannel(channel) {
    let channelElement = document.importNode(channelTemplate.content, true).firstElementChild;
    channelElement.textContent = channel.channelName;
    if (channel.channelType === api.CHANNEL_TYPE_VOICE) {
        channelElement.innerHTML = `<i class="fa-solid fa-phone"></i> ${channelElement.innerHTML}`;
    }
    channelElement.addEventListener("click", () => changeChannel(channel.channelID));
    channelElement.dataset.id = base64.encodeURL(channel.channelID);
    channelElement.dataset.type = channel.channelType;
    channelList.appendChild(channelElement);
}

function changeToPortrait() {
    if (currentChannelID) {
        sidePanel.style.display = "none";
        mainArea.style.display = "flex";
    }
    else {
        sidePanel.style.display = "flex";
        mainArea.style.display = "none";
    }
}

function changeToLandscape() {
    mainArea.style.display = "flex";
    sidePanel.style.display = "flex";
}

function back() {
    mainArea.style.display = "none";
    sidePanel.style.display = "flex";
    history.pushState({}, "", "/app/");
    currentChannelID = null;
}

messageInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        sendMessage();
    }
});
messageInput.addEventListener("input", (event) => {
    let t = event.target.value.length !== 0;
    if (t !== typing) {
        api.setTyping(t, currentChannelID);
        typing = t;
    }
})
messageInput.addEventListener("drop", async (event) => uploadMessageFiles(event.dataTransfer.files));

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

uploadFileButton.addEventListener("click", openUploadDialog);
uploadFileInput.addEventListener("change", async (event) => {
    if (uploadType === "message") {
        return uploadMessageFiles(event.target.files);
    }
    else if (uploadType === "avatar") {
        return uploadAvatarFile(event.target.files[0]);
    }
});

emojiListInput.addEventListener("input", searchEmojiList)

userSettingsButton.addEventListener("click", openUserSettingsMenu);
userSettingsProfileTab.addEventListener("click", () => openUserSettingsTab(userSettingsProfile));
userSettingsProfileChangeAvatar.addEventListener("click", openChangeAvatarDialog);
userSettingsProfileUsername.addEventListener("change", changeUsernameFromSettings);
userSettingsProfileLogout.addEventListener("click", logout);
userSettingsSessionsTab.addEventListener("click", () => {
    openUserSettingsTab(userSettingsSessions);
    api.requestSessions();
});
userSettingsAppearanceTab.addEventListener("click", () => openUserSettingsTab(userSettingsAppearance));
userSettingsAppearanceTheme.addEventListener("change", changeThemeFromSettings);
userSettingsClose.addEventListener("click", closeUserSettingsMenu);

userListManageUsersClose.addEventListener("click", closeManageChannelUsersMenu);
userListManageUsersConfirm.addEventListener("click", changeChannelUsers);

backButton.addEventListener("click", back);

window.addEventListener("drop", (event) => event.preventDefault());
window.addEventListener("dragover", (event) => event.preventDefault());

portraitMedia.addEventListener("change", (event) => {
    if (event.matches) {
        changeToPortrait();
    }
    else {
        changeToLandscape();
    }
});
if (portraitMedia.matches) {
    changeToPortrait();
}