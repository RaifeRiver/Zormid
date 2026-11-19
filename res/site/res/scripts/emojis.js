export let emojis = {};
export let reverseEmojis = {};

export let loadHandler;
export function setLoadHandler(handler) {
    loadHandler = handler;
}

const segmenter = new Intl.Segmenter("en", {granularity: "grapheme"});

export function loadEmojis() {
    fetch("/res/data/emoji_list.json").then(response => response.text()).then(text => {
        emojis = JSON.parse(text);
        reverseEmojis = Object.fromEntries(Object.entries(emojis).map(([key, value]) => [value, key]));
        if (loadHandler) {
            loadHandler();
        }
    });
}

export function toEmojis(text) {
    let result = "";
    let emoji = null;
    for (let i = 0; i < text.length; i++) {
        if (emoji !== null) {
            if (text[i] === ':') {
                if (emojis[emoji]) {
                    result += emojis[emoji];
                }
                else {
                    result += ':';
                    result += emoji;
                    result += ':';
                }
                emoji = null;
            }
            else if (text[i] !== ' ' && text[i] !== '\n') {
                emoji += text[i];
            }
            else {
                result += ':';
                result += emoji;
                result += text[i];
                emoji = null;
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
    for (let {segment} of segmenter.segment(text)) {
        if (reverseEmojis[segment]) {
            result += `:${reverseEmojis[segment]}:`;
        }
        else {
            result += segment;
        }
    }
    return result;
}