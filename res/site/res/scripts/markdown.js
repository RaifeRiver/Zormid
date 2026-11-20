const LIST_BULLET = 0;
const LIST_NUMBERED = 1;

function processInline(markdown) {
    markdown = markdown.replaceAll("&", "&amp;");
    markdown = markdown.replaceAll("<", "&lt;");
    markdown = markdown.replaceAll(">", "&gt;");
    markdown = markdown.replaceAll("\\\\", "&bsol;");
    markdown = markdown.replaceAll("\\`", "&grave;");
    markdown = markdown.replaceAll("\\*", "&ast;");
    markdown = markdown.replaceAll("\\_", "&lowbar;");
    markdown = markdown.replaceAll("\\{", "&lbrace;");
    markdown = markdown.replaceAll("\\}", "&rbrace;");
    markdown = markdown.replaceAll("\\[", "&lbrack;");
    markdown = markdown.replaceAll("\\]", "&rbrack;");
    markdown = markdown.replaceAll("\\&lt;", "&lt;");
    markdown = markdown.replaceAll("\\&gt;", "&gt;");
    markdown = markdown.replaceAll("\\(", "&lpar;");
    markdown = markdown.replaceAll("\\)", "&rpar;");
    markdown = markdown.replaceAll("\\#", "&num;");
    markdown = markdown.replaceAll("\\+", "&plus;");
    markdown = markdown.replaceAll("\\-", "&#45;");
    markdown = markdown.replaceAll("\\.", "&period;");
    markdown = markdown.replaceAll("\\!", "&excl;");
    markdown = markdown.replace("\\|", "&mid;");
    markdown = markdown.replace(/`(.*)`/g, "<code>$1</code>");
    markdown = markdown.replace(/\*\*(.*)\*\*/g, "<strong>$1</strong>");
    markdown = markdown.replace(/__(.*)__/g, "<strong>$1</strong>");
    markdown = markdown.replace(/\*(.*)\*/g, "<em>$1</em>");
    markdown = markdown.replace(/_(.*)_/g, "<em>$1</em>");
    markdown = markdown.replace(/~~(.*)~~/g, "<s>$1</s>");
    markdown = markdown.replace(/(?<!\()(https?:\/\/.*)/g, "<a href=\"$1\">$1</a>");
    markdown = markdown.replace(/\[(.*)]\((.*)\)/g, "<a href=\"$2\">$1</a>");
    markdown = markdown.replace(/&lt;(.+)&gt;/g, "<a href=\"$1\">$1</a>");
    for (let i = 0; i < markdown.length; i++) {
        let startIndex = markdown.indexOf("[", i);
        let endIndex = markdown.indexOf("]", startIndex);
        if (endIndex === -1 || markdown.charAt(endIndex + 1) !== "{") {
            continue;
        }
        let formattingEndIndex = markdown.indexOf("}", endIndex);
        let text = markdown.substring(startIndex + 1, endIndex);
        let formatting = markdown.substring(endIndex + 2, formattingEndIndex);
        let style = "";
        for (let format of formatting.split(",")) {
            let parts = format.split("=", 2);
            if (parts.length < 2) {
                continue;
            }
            if (parts[0] === "color" || parts[0] === "colour") {
                style += `color:${parts[1]};`;
            }
            else if (parts[0] === "background") {
                style += `background:${parts[1]};`;
            }
        }
        markdown = markdown.slice(0, startIndex) + `<span style="${style}">${text}</span>` + markdown.slice(formattingEndIndex + 1);
    }
    return markdown;
}

export function convertToHTML(markdown) {
    let html = "";
    let lists = [];
    let blockquote = false;
    let codeblock = false;
    let endList = () => {
        if (lists.at(-1).type === LIST_NUMBERED) {
            html += "</ol>";
        }
        else {
            html += "</ul>";
        }
        lists.pop();
    };
    let endBlockquote = () => {
        if (blockquote) {
            html += "</blockquote>";
            blockquote = false;
        }
    };
    for (let line of markdown.split("\n")) {
        if (/^ *[0-9]+\. /gm.test(line.trimStart())) {
            line = line.replaceAll("\t", "    ");
            let indent = line.length - line.trimStart().length;
            let number = parseInt(line.substring(0, line.indexOf(".")).trimStart());
            let list = lists.at(-1);
            if (list && list.type === LIST_NUMBERED) {
                if (list.indent === indent) {
                    if (list.lastNumber < number) {
                        list.lastNumber = number;
                        html += `<li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
                    }
                    else {
                        endList();
                        lists.push({type: LIST_NUMBERED, indent: indent, lastNumber: number});
                        html += `<ol type="1"><li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
                    }
                }
                else if (list.indent > indent) {
                    while (list && list.indent > indent) {
                        endList();
                        list = lists.at(-1);
                    }
                    if (list && list.type === LIST_NUMBERED && list.indent === indent) {
                        list.lastNumber = number;
                        html += `<li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
                    }
                    else {
                        if (list && list.indent === indent) {
                            endList();
                        }
                        lists.push({type: LIST_NUMBERED, indent: indent, lastNumber: number});
                        html += `<ol type="1"><li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
                    }
                }
                else if (list.indent < indent) {
                    lists.push({type: LIST_NUMBERED, indent: indent, lastNumber: number});
                    html += `<ol type="1"><li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
                }
            }
            else if (list && list.indent > indent) {
                line = line.replaceAll("\t", "    ");
                while (list && list.indent > indent) {
                    endList();
                    list = lists.at(-1);
                }
                if (list && list.indent === indent) {
                    if (list.type !== LIST_NUMBERED) {
                        endList();
                        list = lists.at(-1);
                        lists.push({type: LIST_NUMBERED, indent: indent, lastNumber: number});
                        html += `<ol type="1"><li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
                    }
                    else {
                        list.lastNumber = number;
                        html += `<li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
                    }
                }
                else {
                    lists.push({type: LIST_NUMBERED, indent: indent, lastNumber: number});
                    html += `<ol type="1"><li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
                }
            }
            else if (list && list.indent === indent) {
                endList();
                lists.push({type: LIST_NUMBERED, indent: indent, lastNumber: number});
                html += `<ol type="1"><li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
            }
            else {
                lists.push({type: LIST_NUMBERED, indent: indent, lastNumber: number});
                html += `<ol type="1"><li value=${number}>${processInline(line.substring(line.indexOf(".") + 1))}</li>`;
            }
            endBlockquote();
        }
        else if (/^ *[-+*] /gm.test(line)) {
            let indent = line.length - line.trimStart().length;
            let list = lists.at(-1);
            if (list && list.type === LIST_BULLET) {
                if (list.indent === indent) {
                    html += `<li>${processInline(line.substring(indent + 2))}</li>`;
                }
                else if (list.indent > indent) {
                    while (list && list.indent > indent) {
                        endList();
                        list = lists.at(-1);
                    }
                    if (list && list.type === LIST_BULLET && list.indent === indent) {
                        html += `<li>${processInline(line.substring(indent + 2))}</li>`;
                    }
                    else {
                        if (list && list.indent === indent) {
                            endList();
                        }
                        lists.push({type: LIST_BULLET, indent: indent});
                        html += `<ul><li>${processInline(line.substring(indent + 2))}</li>`;
                    }
                }
                else if (list.indent < indent) {
                    lists.push({type: LIST_BULLET, indent: indent});
                    html += `<ul><li>${processInline(line.substring(indent + 2))}</li>`;
                }
            }
            else if (list && list.indent > indent) {
                while (list && list.indent > indent) {
                    endList();
                    list = lists.at(-1);
                }
                if (list && list.indent === indent) {
                    if (list.type !== LIST_BULLET) {
                        endList();
                        list = lists.at(-1);
                        lists.push({type: LIST_BULLET, indent: indent});
                        html += `<ul><li>${processInline(line.substring(indent + 2))}</li>`;
                    }
                    else {
                        html += `<li>${processInline(line.substring(indent + 2))}</li>`;
                    }
                }
                else {
                    lists.push({type: LIST_BULLET, indent: indent});
                    html += `<ul><li>${processInline(line.substring(indent + 2))}</li>`;
                }
            }
            else if (list && list.indent === indent) {
                endList();
                lists.push({type: LIST_BULLET, indent: indent});
                html += `<ul><li>${processInline(line.substring(indent + 2))}</li>`;
            }
            else {
                lists.push({type: LIST_BULLET, indent: indent});
                html += `<ul><li>${processInline(line.substring(indent + 2))}</li>`;
            }
            endBlockquote();
        }
        else if (line.startsWith("> ")) {
            if (!blockquote) {
                html += `<blockquote>${processInline(line.substring(2))}`;
                blockquote = true;
            }
            else {
                html += `\n${processInline(line.substring(2))}`;
            }
        }
        else if (line.startsWith("```")) {
            if (!codeblock) {
                html += `<code>`;
                codeblock = true;
            }
            else {
                html += `</code>${processInline(line.substring(3))}`;
                codeblock = false;
            }
        }
        else {
            while (lists.length > 0) {
                endList();
            }
            endBlockquote();
            if (line.startsWith("###### ")) {
                html += `<h6>${processInline(line.substring(7))}</h6>`;
            }
            else if (line.startsWith("##### ")) {
                html += `<h5>${processInline(line.substring(6))}</h5>`;
            }
            else if (line.startsWith("#### ")) {
                html += `<h4>${processInline(line.substring(5))}</h4>`;
            }
            else if (line.startsWith("### ")) {
                html += `<h3>${processInline(line.substring(4))}</h3>`;
            }
            else if (line.startsWith("## ")) {
                html += `<h2>${processInline(line.substring(3))}</h2>`;
            }
            else if (line.startsWith("# ")) {
                html += `<h1>${processInline(line.substring(2))}</h1>`;
            }
            else if (line.trim().length > 1) {
                if (codeblock) {
                    html += `\n${line}`;
                }
                else {
                    html += `<p>${processInline(line)}</p>`;
                }
            }
        }
    }
    while (lists.length > 0) {
        endList();
    }
    return html;
}