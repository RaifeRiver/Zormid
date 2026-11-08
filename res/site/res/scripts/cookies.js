export function hasCookie(name) {
    return document.cookie.split(';').some(c => c.trim().startsWith(name + '='));
}

export function getCookie(name) {
    const value = "; " + document.cookie;
    const parts = value.split("; " + name + "=");
    if (parts.length === 2) {
        return parts[parts.length - 1].split(';')[0];
    }
    return undefined;
}

export function setCookie(name, value) {
    document.cookie = name + "=" + value + ";";
}

export function deleteCookie(name) {
    document.cookie = name + "=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;";
}