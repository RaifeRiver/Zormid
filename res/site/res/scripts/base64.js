export function encodeURL(str) {
    return btoa(str).replaceAll("+", "-").replaceAll("/", "_").replaceAll("=", "");
}

export function decodeURL(str) {
    while (str.length % 4 !== 0) {
        str += "=";
    }
    return atob(str.replaceAll("-", "+").replaceAll("_", "/"));
}