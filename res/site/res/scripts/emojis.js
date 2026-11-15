let emojis = {};
let reverseEmojis = {};

fetch("/res/data/emoji_list.json").then(response => response.text()).then(text => {
    emojis = JSON.parse(text);
    reverseEmojis = Object.fromEntries(Object.entries(emojis).map(([key, value]) => [value, key]));
});

export function toEmojis(text) {
    let result = "";
    let emoji = null;
    for (let i = 0; i < text.length; i++) {
        if (emoji !== null) {
            if (text[i] === ':') {
                result += emojis[emoji];
                emoji = null;
            }
            else {
                emoji += text[i];
            }
        }
        else if (text[i] === ':') {
            emoji = "";
        }
        else {
            result += text[i];
        }
    }
    if (emoji) {
        result += ':';
        result += emoji;
    }
    return result;
}

export function toPlainText(text) {
    let result = "";
    for (let c of text) {
        if (reverseEmojis[c]) {
            result += ':';
            result += reverseEmojis[c];
            result += ':';
        }
        else {
            result += c;
        }
    }
    return result;
}