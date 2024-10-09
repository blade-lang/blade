import mail

var data = mail.parse('Delivered-To: eqliqandfriends@gmail.com
Received: by 2002:a05:6f02:20e:b0:23:bff0:67f0 with SMTP id 14csp911783rcn;
        Sat, 24 Sep 2022 03:12:40 -0700 (PDT)
X-Received: by 2002:ad4:5aaf:0:b0:4a8:a817:e00d with SMTP id u15-20020ad45aaf000000b004a8a817e00dmr10249309qvg.18.1664014360215;
        Sat, 24 Sep 2022 03:12:40 -0700 (PDT)
ARC-Seal: i=1; a=rsa-sha256; t=1664014360; cv=none;
        d=google.com; s=arc-20160816;
        b=Rpl7eCMZxaMwn9my7wweR2HmO8JIILRSddUxSZ+R4zGIZEYbeCzif14MbVeqT/ejGF
         NltYrzeyMZdpl0mZ4HSnJOla8pOtFd36+axEYgiA5Cy95E4dBLLDPCeCmzNjONiw5Z5c
         EY0fLNffQentaQPftf5oF0yb3mAR/euqvwew1RbL2sxFfX06AbtjhBDJXoRP+kk8gwSU
         1pbsP4UZXa0VaL65s2CER7OTjflSbEb7F+5mom0IBt1pKdKtDA+6C1RqAus2t97gs6oA
         cM0ccY1txVVuqmvzeoitNrEK/ukDBEpQEh2oDaxhyoVV/DtIsYx7zsWUeL+uvWb/g/8s
         FvaA==
ARC-Message-Signature: i=1; a=rsa-sha256; c=relaxed/relaxed; d=google.com; s=arc-20160816;
        h=to:from:subject:date:message-id:references:reply-to:mime-version
         :dkim-signature;
        bh=bqUPMi8/QkKanRlKc6rAkjGVhMv4S3wQsBRAg2ieIUc=;
        b=zleQOw6Rh7vQE87cUp9Z8tPH8xt6sISZ6C8MB/H4Ca207vl/RwU1iFbWN65z9J8vLq
         3neJthn2NpwYmdR7+JVFjKnjSdTE26lAr5efr7iAlJUs97Cx/lxSeGMM/hOgplO3eHf1
         AG5VMOkL5dPy8cHJDYsAMW8OJo0/+yCWDZrPkn7zZHlcXiCN3hkFugCsyoPVn9uyGhOb
         8WB3N4dzmZkgp88/kkrLdhgEN6YgcJZwGp3wiWum3OTjgfqBQ8+G8QYXIz9UM0WEdriL
         oQag4MbzjvYUxJ3jW74ir/RYlXGwO2c7fw0C1q4/wRRQBwhIuOj6vZsUu7ffbwm+TAGv
         bLuQ==
ARC-Authentication-Results: i=1; mx.google.com;
       dkim=pass header.i=@google.com header.s=20210112 header.b=eE+DBZgF;
       spf=pass (google.com: domain of 3f9guyxckakiftkxg-ujctgu-fo-pqtgrnaiqqing.eqo@doclist.bounces.google.com designates 209.85.220.69 as permitted sender) smtp.mailfrom=3F9guYxcKAKIFTKXG-UJCTGU-FO-PQTGRNaIQQING.EQO@doclist.bounces.google.com;
       dmarc=pass (p=REJECT sp=REJECT dis=NONE) header.from=google.com
Return-Path: <3F9guYxcKAKIFTKXG-UJCTGU-FO-PQTGRNaIQQING.EQO@doclist.bounces.google.com>
Received: from mail-sor-f69.google.com (mail-sor-f69.google.com. [209.85.220.69])
        by mx.google.com with SMTPS id az44-20020a05620a172c00b006bacbde12e6sor3011320qkb.68.2022.09.24.03.12.39
        for <eqliqandfriends@gmail.com>
        (Google Transport Security);
        Sat, 24 Sep 2022 03:12:40 -0700 (PDT)
Received-SPF: pass (google.com: domain of 3f9guyxckakiftkxg-ujctgu-fo-pqtgrnaiqqing.eqo@doclist.bounces.google.com designates 209.85.220.69 as permitted sender) client-ip=209.85.220.69;
Authentication-Results: mx.google.com;
       dkim=pass header.i=@google.com header.s=20210112 header.b=eE+DBZgF;
       spf=pass (google.com: domain of 3f9guyxckakiftkxg-ujctgu-fo-pqtgrnaiqqing.eqo@doclist.bounces.google.com designates 209.85.220.69 as permitted sender) smtp.mailfrom=3F9guYxcKAKIFTKXG-UJCTGU-FO-PQTGRNaIQQING.EQO@doclist.bounces.google.com;
       dmarc=pass (p=REJECT sp=REJECT dis=NONE) header.from=google.com
DKIM-Signature: v=1; a=rsa-sha256; c=relaxed/relaxed;
        d=google.com; s=20210112;
        h=to:from:subject:date:message-id:references:reply-to:mime-version
         :from:to:cc:subject:date;
        bh=bqUPMi8/QkKanRlKc6rAkjGVhMv4S3wQsBRAg2ieIUc=;
        b=eE+DBZgFSKE9ASNKtj5mMcjma9AUs/fGVgxOR63pGzwu9gDhZFn1LVGh42/dnOukk5
         PDD3czgNqJWPHaiUpBj/1KA733L4Dv95dyDNwjySallNdC2Vvgcib+Oo8MeLgrNblQBl
         tUqK4/6ctqXQziDEmy9VIyjam3VcLQHHI5q+Y8VvabGInWaa4vf4MkKUTuV5BQBlGGKB
         FRYZGu+8VV8QiQjiAoqmDbvlIxRuCsF5tYRhiMP2q3RvXp/Tz6qaS9sqliYjnIewBWPq
         kol9Zduoq61oo6GLFFKOSLT6i98r3SLtBiT8QWKleNlyoHTQnYL6mRCOlcBRTo4/aTAw
         4izA==
X-Google-DKIM-Signature: v=1; a=rsa-sha256; c=relaxed/relaxed;
        d=1e100.net; s=20210112;
        h=to:from:subject:date:message-id:references:reply-to:mime-version
         :x-gm-message-state:from:to:cc:subject:date;
        bh=bqUPMi8/QkKanRlKc6rAkjGVhMv4S3wQsBRAg2ieIUc=;
        b=VPmnZGhWb041YV24mZ9oroyFbB2RGwZZRL3Ds+llF3N2ou7H8ZBeA8DaNy0syjfyKx
         2CAwadTlTHBXCf+cQpx+SdoNlIUFnYdeVuh2XRv9Vrt/cNTqXPzLGFlYlgMCPhZNojrI
         KzU9iAg6xm7lCKVKrpahRScAvZWmOWO/bWPd8FhLLDGnIqf0csaOwRqpuDLigvLlHII1
         pUXt+ewq1X5BWkMcHWO1x+QNzsAr+HAoIYROhSMD2VK/EEkX8r8MsLf2CHZrK07WENZA
         VEkCaforE8bjJAM/y5bVFkkEJ2PPNLBPSd3YjDbeRl7p/5pCYYbBLlcmaA03XRiNNSZs
         VTmQ==
X-Gm-Message-State: ACrzQf1KyO/HTLtOf++E7S16Ch6Q1jEDnlkCXzBPyZz2hcos9YXjlkS8 1x4DJBaiNw/INJoVyQXJNpXKxGkkHKQ=
X-Google-Smtp-Source: AMsMyM4/tmby9PXqf/5tgxcZapGUSDFmq3JBNT7uvDMBvJOkT0GABWQ4YW1XsRiVRKIk3E6Jr27v4z2Kbzg=
MIME-Version: 1.0
X-Received: by 2002:a05:620a:8014:b0:6ce:3dcf:9c32 with SMTP id ee20-20020a05620a801400b006ce3dcf9c32mr7995870qkb.766.1664014359557; Sat, 24 Sep 2022 03:12:39 -0700 (PDT)
Reply-To: Comfort Inyang <icey.me6@gmail.com>
X-No-Auto-Attachment: 1
References: <862912ce-7a5a-47ae-a743-1f420d8f5362@docs-share.google.com>
Message-ID: <000000000000cd7cf305e9698884@google.com>
Date: Sat, 24 Sep 2022 10:12:39 +0000
Subject: Spreadsheet shared with you: "1st Principles Coding Mentorship program (Responses)"
From: "Comfort Inyang (via Google Sheets)" <drive-shares-dm-noreply@google.com>
To: eqliqandfriends@gmail.com
Content-Type: multipart/alternative; boundary="000000000000cd7ce005e9698881"

--000000000000cd7ce005e9698881
Content-Type: text/plain; charset="UTF-8"; format=flowed; delsp=yes

I\'ve shared an item with you:

1st Principles Coding Mentorship program (Responses)
https://docs.google.com/spreadsheets/d/1icYCsnRoKBvxpYXGCIPwzkUuc9BWlXLkN_Q9R01CwOM/edit?usp=sharing&ts=632ed817

It\'s not an attachment -- it\'s stored online. To open this item, just click  
the link above.

--000000000000cd7ce005e9698881
Content-Type: text/x-amp-html; charset="UTF-8"
Content-Transfer-Encoding: quoted-printable

<!doctype html><html amp4email><head><meta charset=3D"utf-8"><script async =
src=3D"https://cdn.ampproject.org/v0.js"></script><script async custom-elem=
ent=3D"amp-list" src=3D"https://cdn.ampproject.org/v0/amp-list-0.1.js"></sc=
ript><script async custom-template=3D"amp-mustache" src=3D"https://cdn.ampp=
roject.org/v0/amp-mustache-0.2.js"></script><script async custom-element=3D=
"amp-form" src=3D"https://cdn.ampproject.org/v0/amp-form-0.1.js"></script><=
script async custom-element=3D"amp-bind" src=3D"https://cdn.ampproject.org/=
v0/amp-bind-0.1.js"></script><script async custom-element=3D"amp-timeago" s=
rc=3D"https://cdn.ampproject.org/v0/amp-timeago-0.1.js"></script><style amp=
4email-boilerplate>body{visibility:hidden}</style><style amp-custom>.materi=
al-button{-webkit-appearance: none; cursor: pointer; outline: none;}.materi=
al-button:focus {outline: 1px solid transparent;}.material-button:disabled{=
cursor: initial; outline: none;}.material-button-filled{background-color: #=
1a73e8; color: #fff;}.material-button-filled:hover{background-color: #1b55c=
9; box-shadow: 0 1px 2px 0 rgba(60, 64, 67, 0.3), 0 1px 3px 1px rgba(60,64,=
67,0.15);}.material-button-filled:focus{background-color: #1b5fb9; box-shad=
ow: 0 1px 2px 0 rgba(60, 64, 67, 0.3), 0 1px 3px 1px rgba(60,64,67,0.15);}.=
material-button-filled:active{background-color: #1b63c1; box-shadow: 0 1px =
2px 0 rgba(60, 64, 67, 0.3), 0 2px 6px 2px rgba(60, 64, 67, 0.15);}.materia=
l-button-filled:disabled{background-color: rgba(60, 64, 67, .12); color: rg=
ba(60, 64, 67, .38);}.material-button-transparent{background-color: transpa=
rent; color: #1a73e8;}.material-button-transparent:hover{background-color: =
rgba(26, 115, 232, .04);}.material-button-transparent:focus{background-colo=
r: rgba(26, 115, 232, .12);}.material-button-transparent:active{background-=
color: rgba(26, 115, 232, .12); box-shadow: 0 1px 3px 1px rgba(60, 64, 67, =
.15);}.material-button-transparent:disabled{background-color: transparent; =
color: #3c4043; opacity: 0.38;}@media screen and (max-width: 600px){.conten=
t-spacer{height: 24px;}.content-spacer-small{height: 12px;}}@media screen a=
nd (min-width: 601px){.content-spacer{height: 32px;}.content-spacer-small{h=
eight: 20px;}}.dynamic-content-container-wrapper {margin-left: -6px; table-=
layout: fixed; width: calc(100% + 12px);}.dynamic-content-container-wrapper=
 * {hyphens: auto; overflow-wrap: break-word; word-wrap: break-word; word-b=
reak: break-word;}#dynamic-content-container, .thumbnail-link {border: 1px =
solid #DADCE0; border-radius: 8px; box-sizing: border-box;}#dynamic-content=
-container {display: inline-block; max-width: 100%; padding: 20px; width: 4=
05px;}#dynamic-content-container > * + * {margin-top: 18px;}.dynamic-conten=
t-heading {display: flex; flex-direction: row;}.dynamic-content-heading > *=
 + * {margin-left: 10px;}#star-form, #star-button {height: 22px; position: =
relative; width: 22px;}#star-button {background: none; border: none; displa=
y: block; outline: none; z-index: 1;}#star-button[disabled] {opacity: 0.4;}=
#star-button:not([disabled]) {cursor: pointer;}#star-button:not([disabled])=
:hover + .star-button-circle, #star-button:not([disabled]):focus + .star-bu=
tton-circle{display: block; outline: 1px solid transparent;}.star-button-ci=
rcle {background-color: #F1F3F4; border-radius: 50%; display: none; height:=
 32px; left: 50%; position: absolute; top: 50%; transform: translate(-50%, =
-50%); width: 32px;}.unstarred-icon, .starred-icon {bottom: 0; left: 0; pos=
ition: absolute; right: 0; top: 0; visibility: hidden;}#star-button.starred=
 > .starred-icon, #star-button.unstarred > .unstarred-icon {visibility: vis=
ible;}#star-error-message {color: #D93025; font: 400 14px/16px Roboto, Aria=
l, Helvetica, sans-serif; margin-top: 5px;}.display-none {display: none;}.t=
humbnail-link {display: block; overflow: hidden; position: relative;}.thumb=
nail-open {align-items: center; background-color: #202124; bottom: 0; color=
: white; display: none; font: 400 14px/16px Google Sans, Roboto, Arial, Hel=
vetica, sans-serif; justify-content: center; left: 0; letter-spacing: 0.15p=
x; opacity: 65%; position: absolute; right: 0; top: 0; z-index: 1;}.thumbna=
il-link:hover > .thumbnail-open, .thumbnail-link:focus > .thumbnail-open {d=
isplay: flex;}amp-img.cover img {object-fit: cover; object-position: 0 0;}.=
large-icon-container {align-items: center; display: flex; height: 100%; jus=
tify-content: center;}.dynamic-message {align-items: center; display: flex;=
 flex-direction: row;}.dynamic-message > amp-img {flex-shrink: 0;}.dynamic-=
message > span {color: #5F6368; font: 400 14px/18px Roboto, Arial, Helvetic=
a, sans-serif; letter-spacing: 0; margin-left: 18px;}.dynamic-message-summa=
ry {margin-left: 16px; margin-top: 4px;}.blue-text-header {color: #1a73e8; =
font-weight: 500;}.horizontal-rule-wrapper {margin-top: 16px;}#amp-timeago =
{display: inline;}</style></head><body><amp-list diffable binding=3D"refres=
h-evaluate" src=3D"https://drive.google.com/sharing/dynamicmail/fetchinvite=
?ts=3D632ed817&amp;shareService=3Dritz&amp;hl=3Den&amp;id=3D1icYCsnRoKBvxpY=
XGCIPwzkUuc9BWlXLkN_Q9R01CwOM&amp;dynamicEmailToken=3DAYSVQUVLH3lYgJSP5notO=
KC6lL6f1CEBZGCzFoO989feWCYuAN2eI2SbPqtB8I-6Ld8TiDCs8ySFjyebFLYL3EI1l2F5MV9p=
8b4xNqZTxqmHlMLpF7llUVS-Iod3vOX14sOVxQNoVtrZjQ%3D%3D&amp;resourcekey&amp;bu=
ildLabel=3Ddrive.explorer_20220916.00_p0" single-item items=3D"." layout=3D=
"container"><template type=3D"amp-mustache"><table style=3D"border-collapse=
: collapse; width: 100%; background-color: white; text-align: center;" role=
=3D"presentation"><tr><td style=3D"padding: 24px 0 16px 0;"><table style=3D=
"border-collapse: collapse;font-family: Roboto, Arial, Helvetica, sans-seri=
f;hyphens: auto; overflow-wrap: break-word; word-wrap: break-word; word-bre=
ak: break-word;display: inline-block; width: 90%;max-width: 700px;min-width=
: 280px; text-align: left;" role=3D"presentation"><tr><td style=3D"padding:=
 0;"><table style=3D"width:100%; border: 1px solid #dadce0; border-radius: =
8px; border-spacing: 0; table-layout:fixed; border-collapse: separate;" rol=
e=3D"presentation"><tr><td style=3D"padding: 4.5%;" dir=3D"ltr"><div style=
=3D"margin-bottom:32px;font-family: Google Sans, Roboto, Arial, Helvetica, =
sans-serif; font-style: normal; font-size: 28px; line-height: 36px; color: =
#3c4043;">Comfort Inyang shared a spreadsheet</div><table style=3D"border-c=
ollapse: collapse;font-family: Roboto, Arial, Helvetica, sans-serif; font-s=
ize:16px; line-height:24px; color:#202124; letter-spacing:0.1px; table-layo=
ut:fixed; width:100%; overflow-wrap: break-word;" role=3D"presentation"><tr=
><td style=3D"padding: 0; vertical-align:top; width:50px;"><!--[if mso]><v:=
oval xmlns:v=3D"urn:schemas-microsoft-com:vml" xmlns:w=3D"urn:schemas-micro=
soft-com:office:word" style=3D"height:50px;width:50px;" fill=3D"t" stroke=
=3D"f"><v:fill type=3D"frame" src=3D"https://ssl.gstatic.com/s2/profiles/im=
ages/silhouette64.png" alt=3D"Unknown profile photo" style=3D"height:50px;w=
idth:50px;"/></v:oval><![endif]--><div style=3D"mso-hide:all;"><amp-img sty=
le=3D"border-radius:50%; display:block;" width=3D"50" height=3D"50" src=3D"=
https://ssl.gstatic.com/s2/profiles/images/silhouette64.png" alt=3D"Unknown=
 profile photo"></amp-img></div></td><td style=3D"padding: 0; vertical-alig=
n:top; padding-left:12px;"><div style=3D"padding-top:12px;">Comfort Inyang =
(<a href=3D"mailto:icey.me6@gmail.com" style=3D"color:inherit;text-decorati=
on:none">icey.me6@gmail.com</a>) has invited you to <b>edit</b> the followi=
ng spreadsheet:</div></td></tr></table><table class=3D"dynamic-content-cont=
ainer-wrapper" role=3D"presentation"><tr style=3D"height: 20px;"></tr><tr><=
td id=3D"dynamic-content-container" role=3D"presentation" tabindex=3D"0"><d=
iv class=3D"dynamic-content-heading"><a href=3D"https://docs.google.com/spr=
eadsheets/d/1icYCsnRoKBvxpYXGCIPwzkUuc9BWlXLkN_Q9R01CwOM/edit?usp=3Dsharing=
_eil_se_dm&amp;ts=3D632ed817" target=3D"_blank" style=3D"color: #3c4043; di=
splay: inline-block; max-width: 100%; text-decoration: none; vertical-align=
: top;display: flex; flex-direction: column; justify-content: center;"><div=
 style=3D"line-height: 18px; overflow: hidden; text-overflow: ellipsis;disp=
lay: flex;"><span style=3D"display: inline-block; vertical-align: top; min-=
width: 26px; width: 26px;"><amp-img src=3D"https://ssl.gstatic.com/docs/doc=
list/images/mediatype/icon_1_spreadsheet_x64.png" width=3D"18" height=3D"18=
" style=3D"vertical-align: top;" role=3D"presentation"></amp-img></span><sp=
an style=3D"font: 500 14px/18px Google Sans, Roboto, Arial, Helvetica, sans=
-serif; display: inline; letter-spacing: 0.2px;">1st Principles Coding Ment=
orship program (Responses)</span></div></a><form id=3D"star-form" action-xh=
r=3D"https://drive.google.com/sharing/dynamicmail/star?ts=3D632ed817&amp;sh=
areService=3Dritz&amp;hl=3Den&amp;id=3D1icYCsnRoKBvxpYXGCIPwzkUuc9BWlXLkN_Q=
9R01CwOM&amp;dynamicEmailToken=3DAYSVQUVLH3lYgJSP5notOKC6lL6f1CEBZGCzFoO989=
feWCYuAN2eI2SbPqtB8I-6Ld8TiDCs8ySFjyebFLYL3EI1l2F5MV9p8b4xNqZTxqmHlMLpF7llU=
VS-Iod3vOX14sOVxQNoVtrZjQ%3D%3D&amp;resourcekey&amp;buildLabel=3Ddrive.expl=
orer_20220916.00_p0" method=3D"post" on=3D"submit: dynamic-content-containe=
r.focus, AMP.setState({starButton: {value: starButton =3D=3D null ? {{starr=
edByRequester}} : starButton.value, inProgress: true, error: false}}); subm=
it-success: AMP.setState({starButton: {value: !starButton.value, inProgress=
: false, error: false}}); submit-error: AMP.setState({starButton: {inProgre=
ss: false, error: true}});"><input type=3D"hidden" name=3D"starred" value=
=3D"{{#starredByRequester}}false{{/starredByRequester}}{{^starredByRequeste=
r}}true{{/starredByRequester}}" data-amp-bind-value=3D"starButton.value ? \'=
false\' : \'true\'"><button id=3D"star-button" class=3D"{{#starredByRequester}=
}starred{{/starredByRequester}}{{^starredByRequester}}unstarred{{/starredBy=
Requester}}" data-amp-bind-class=3D"starButton.value ? \'starred\' : \'unstarr=
ed\'" type=3D"submit" aria-live=3D"polite" aria-label=3D"{{#starredByRequest=
er}}Starred{{/starredByRequester}}{{^starredByRequester}}Unstarred{{/starre=
dByRequester}}" data-amp-bind-aria-label=3D"starButton.value ? \'Starred\' : =
\'Unstarred\'" title=3D"Star in Drive" data-amp-bind-disabled=3D"starButton.i=
nProgress"><amp-img src=3D"https://fonts.gstatic.com/s/i/googlematerialicon=
s/star_border/v8/gm_grey-48dp/1x/gm_star_border_gm_grey_48dp.png" layout=3D=
"fixed" width=3D"22" height=3D"22" role=3D"presentation" aria-hidden=3D"tru=
e" class=3D"unstarred-icon"></amp-img><amp-img src=3D"https://fonts.gstatic=
.com/s/i/googlematerialicons/star/v8/gm_grey-48dp/1x/gm_star_gm_grey_48dp.p=
ng" layout=3D"fixed" width=3D"22" height=3D"22" role=3D"presentation" aria-=
hidden=3D"true" class=3D"starred-icon"></amp-img></button><div class=3D"sta=
r-button-circle"></div></form></div><div id=3D"star-error-message" class=3D=
"display-none" data-amp-bind-class=3D"starButton.error ? \'\' : \'display-none=
\'">Something went wrong. Try again</div><a href=3D"https://docs.google.com/=
spreadsheets/d/1icYCsnRoKBvxpYXGCIPwzkUuc9BWlXLkN_Q9R01CwOM/edit?usp=3Dshar=
ing_eil_se_dm&amp;ts=3D632ed817" target=3D"_blank" class=3D"thumbnail-link"=
><amp-layout layout=3D"responsive" width=3D"386" height=3D"202">{{#thumbnai=
lUrl}}<amp-img class=3D"cover" src=3D"{{thumbnailUrl}}" layout=3D"fill"><di=
v class=3D"large-icon-container" fallback><amp-img src=3D"https://drive-thi=
rdparty.googleusercontent.com/256/type/application/vnd.google-apps.spreadsh=
eet" width=3D"80" height=3D"80"></amp-img></div></amp-img>{{/thumbnailUrl}}=
{{^thumbnailUrl}}<div class=3D"large-icon-container"><amp-img src=3D"https:=
//drive-thirdparty.googleusercontent.com/256/type/application/vnd.google-ap=
ps.spreadsheet" width=3D"80" height=3D"80"></amp-img></div>{{/thumbnailUrl}=
}</amp-layout><div class=3D"thumbnail-open">Open</div></a>{{#summaryDescrip=
tion}}<div><div class=3D"dynamic-message"><amp-img src=3D"https://www.gstat=
ic.com/docs/documents/share/images/smart_summary.png" layout=3D"fixed" widt=
h=3D"16" height=3D"16" role=3D"presentation" aria-hidden=3D"true"></amp-img=
><span><span class=3D"blue-text-header">Summary</span></span></div><div cla=
ss=3D"dynamic-message dynamic-message-summary"><span>{{summaryDescription}}=
</span></div><div class=3D"horizontal-rule-wrapper"><div style=3D"height: 1=
px; background-color: #DADCE0;"></div></div></div>{{/summaryDescription}}{{=
#ownerOrCreatorMessage}}<div class=3D"dynamic-message"><amp-img src=3D"http=
s://www.gstatic.com/docs/documents/share/images/person.png" layout=3D"fixed=
" width=3D"16" height=3D"16" role=3D"presentation" aria-hidden=3D"true"></a=
mp-img><span>{{ownerOrCreatorMessage}}</span></div>{{/ownerOrCreatorMessage=
}}{{#lastEdited}}<div class=3D"dynamic-message"><amp-img src=3D"https://www=
.gstatic.com/docs/documents/share/images/clock.png" layout=3D"fixed" width=
=3D"16" height=3D"16" role=3D"presentation" aria-hidden=3D"true"></amp-img>=
<span>Last edited by {{lastEdited.editor}} <amp-timeago id=3D"amp-timeago" =
layout=3D"fixed-height" height=3D"1" datetime=3D"{{lastEdited.datetime}}" l=
ocale=3D"en">{{lastEdited.datetime}}</amp-timeago></span></div>{{/lastEdite=
d}}</td></tr></table><table style=3D"border-collapse: collapse;" role=3D"pr=
esentation"><tr style=3D"height: 32px"><td></td></tr></table><div><a href=
=3D"https://docs.google.com/spreadsheets/d/1icYCsnRoKBvxpYXGCIPwzkUuc9BWlXL=
kN_Q9R01CwOM/edit?usp=3Dsharing_eip_se_dm&amp;ts=3D632ed817" class=3D"mater=
ial-button material-button-filled" target=3D"_blank" tabindex=3D"0" role=3D=
"button" style=3D"padding: 0 24px;font: 500 14px/36px Google Sans, Roboto, =
Arial, Helvetica, sans-serif; border: none; border-radius: 18px; box-sizing=
: border-box; display: inline-block; letter-spacing: .25px; min-height: 36p=
x; text-align: center; text-decoration: none;">Open</a></div><table style=
=3D"border-collapse: collapse;" role=3D"presentation"><tr style=3D"height: =
32px"><td></td></tr></table><div style=3D"font-size: 12px; color: #5F6368">=
If you don\'t want to receive files from this person, <a href=3D"https://dri=
ve.google.com/drive/blockuser?blockerEmail=3Deqliqandfriends@gmail.com&amp;=
blockeeEmail=3Dicey.me6@gmail.com&amp;usp=3Dsharing_eib_se_dm" target=3D"_b=
lank" style=3D"color: #1a73e8; text-decoration: none;">block the sender</a>=
 from Drive</div></td></tr></table><table style=3D"border-collapse: collaps=
e; width: 100%;" role=3D"presentation"><tr><td style=3D"padding: 24px 4.5%"=
><table style=3D"border-collapse: collapse; width: 100%;" dir=3D"ltr"><tr><=
td style=3D"padding: 0;font-family: Roboto, Arial, Helvetica, sans-serif; c=
olor: #5F6368; width: 100%; font-size: 12px; line-height: 16px; min-height:=
 40px; letter-spacing: .3px;">Google LLC, 1600 Amphitheatre Parkway, Mounta=
in View, CA 94043, USA<br/> You have received this email because <a href=3D=
"mailto:icey.me6@gmail.com" style=3D"color:inherit;text-decoration:none">ic=
ey.me6@gmail.com</a> shared a spreadsheet with you from Google Sheets.</td>=
<td style=3D"padding: 0;padding-left: 20px; min-width: 96px"><a href=3D"htt=
ps://www.google.com/" target=3D"_blank"><amp-img src=3D"https://www.gstatic=
.com/images/branding/googlelogo/2x/googlelogo_grey_tm_color_96x40dp.png" wi=
dth=3D"96" height=3D"40" alt=3D"Google logo"></amp-img></a></td></tr></tabl=
e></td></tr></table></td></tr></table></td></tr></table></template><div rol=
e=3D"list"><table style=3D"border-collapse: collapse; width: 100%; backgrou=
nd-color: white; text-align: center;" role=3D"presentation"><tr><td style=
=3D"padding: 24px 0 16px 0;"><table style=3D"border-collapse: collapse;font=
-family: Roboto, Arial, Helvetica, sans-serif;hyphens: auto; overflow-wrap:=
 break-word; word-wrap: break-word; word-break: break-word;display: inline-=
block; width: 90%;max-width: 700px;min-width: 280px; text-align: left;" rol=
e=3D"presentation"><tr><td style=3D"padding: 0;"><table style=3D"width:100%=
; border: 1px solid #dadce0; border-radius: 8px; border-spacing: 0; table-l=
ayout:fixed; border-collapse: separate;" role=3D"presentation"><tr><td styl=
e=3D"padding: 4.5%;" dir=3D"ltr"><div style=3D"margin-bottom:32px;font-fami=
ly: Google Sans, Roboto, Arial, Helvetica, sans-serif; font-style: normal; =
font-size: 28px; line-height: 36px; color: #3c4043;">Comfort Inyang shared =
a spreadsheet</div><table style=3D"border-collapse: collapse;font-family: R=
oboto, Arial, Helvetica, sans-serif; font-size:16px; line-height:24px; colo=
r:#202124; letter-spacing:0.1px; table-layout:fixed; width:100%; overflow-w=
rap: break-word;" role=3D"presentation"><tr><td style=3D"padding: 0; vertic=
al-align:top; width:50px;"><!--[if mso]><v:oval xmlns:v=3D"urn:schemas-micr=
osoft-com:vml" xmlns:w=3D"urn:schemas-microsoft-com:office:word" style=3D"h=
eight:50px;width:50px;" fill=3D"t" stroke=3D"f"><v:fill type=3D"frame" src=
=3D"https://ssl.gstatic.com/s2/profiles/images/silhouette64.png" alt=3D"Unk=
nown profile photo" style=3D"height:50px;width:50px;"/></v:oval><![endif]--=
><div style=3D"mso-hide:all;"><amp-img style=3D"border-radius:50%; display:=
block;" width=3D"50" height=3D"50" src=3D"https://ssl.gstatic.com/s2/profil=
es/images/silhouette64.png" alt=3D"Unknown profile photo"></amp-img></div><=
/td><td style=3D"padding: 0; vertical-align:top; padding-left:12px;"><div s=
tyle=3D"padding-top:12px;">Comfort Inyang (<a href=3D"mailto:icey.me6@gmail=
.com" style=3D"color:inherit;text-decoration:none">icey.me6@gmail.com</a>) =
has invited you to <b>edit</b> the following spreadsheet:</div></td></tr></=
table><table style=3D"border-spacing:0 4px; table-layout:fixed; width:100%;=
 overflow-wrap: break-word;" role=3D"presentation"><tr style=3D"height:28px=
;"></tr><tr><td style=3D"padding: 0;"><a href=3D"https://docs.google.com/sp=
readsheets/d/1icYCsnRoKBvxpYXGCIPwzkUuc9BWlXLkN_Q9R01CwOM/edit?usp=3Dsharin=
g_eil_se_dm&amp;ts=3D632ed817" target=3D"_blank" style=3D"color: #3c4043; d=
isplay: inline-block; max-width: 100%; text-decoration: none; vertical-alig=
n: top;border: 1px solid #DADCE0; border-radius: 16px; white-space: nowrap;=
"><div style=3D"line-height: 18px; overflow: hidden; text-overflow: ellipsi=
s;padding: 6px 12px;"><span style=3D"display: inline-block; vertical-align:=
 top; min-width: 26px; width: 26px;"><amp-img src=3D"https://ssl.gstatic.co=
m/docs/doclist/images/mediatype/icon_1_spreadsheet_x64.png" width=3D"18" he=
ight=3D"18" style=3D"vertical-align: top;" role=3D"presentation"></amp-img>=
</span><span style=3D"font: 500 14px/18px Google Sans, Roboto, Arial, Helve=
tica, sans-serif; display: inline; letter-spacing: 0.2px;">1st Principles C=
oding Mentorship program (Responses)</span></div></a></td></tr></table><tab=
le style=3D"border-collapse: collapse;" role=3D"presentation"><tr style=3D"=
height: 32px"><td></td></tr></table><div><a href=3D"https://docs.google.com=
/spreadsheets/d/1icYCsnRoKBvxpYXGCIPwzkUuc9BWlXLkN_Q9R01CwOM/edit?usp=3Dsha=
ring_eip_se_dm&amp;ts=3D632ed817" class=3D"material-button material-button-=
filled" target=3D"_blank" tabindex=3D"0" role=3D"button" style=3D"padding: =
0 24px;font: 500 14px/36px Google Sans, Roboto, Arial, Helvetica, sans-seri=
f; border: none; border-radius: 18px; box-sizing: border-box; display: inli=
ne-block; letter-spacing: .25px; min-height: 36px; text-align: center; text=
-decoration: none;">Open</a></div><table style=3D"border-collapse: collapse=
;" role=3D"presentation"><tr style=3D"height: 32px"><td></td></tr></table><=
div style=3D"font-size: 12px; color: #5F6368">If you don\'t want to receive =
files from this person, <a href=3D"https://drive.google.com/drive/blockuser=
?blockerEmail=3Deqliqandfriends@gmail.com&amp;blockeeEmail=3Dicey.me6@gmail=
.com&amp;usp=3Dsharing_eib_se_dm" target=3D"_blank" style=3D"color: #1a73e8=
; text-decoration: none;">block the sender</a> from Drive</div></td></tr></=
table><table style=3D"border-collapse: collapse; width: 100%;" role=3D"pres=
entation"><tr><td style=3D"padding: 24px 4.5%"><table style=3D"border-colla=
pse: collapse; width: 100%;" dir=3D"ltr"><tr><td style=3D"padding: 0;font-f=
amily: Roboto, Arial, Helvetica, sans-serif; color: #5F6368; width: 100%; f=
ont-size: 12px; line-height: 16px; min-height: 40px; letter-spacing: .3px;"=
>Google LLC, 1600 Amphitheatre Parkway, Mountain View, CA 94043, USA<br/> Y=
ou have received this email because <a href=3D"mailto:icey.me6@gmail.com" s=
tyle=3D"color:inherit;text-decoration:none">icey.me6@gmail.com</a> shared a=
 spreadsheet with you from Google Sheets.</td><td style=3D"padding: 0;paddi=
ng-left: 20px; min-width: 96px"><a href=3D"https://www.google.com/" target=
=3D"_blank"><amp-img src=3D"https://www.gstatic.com/images/branding/googlel=
ogo/2x/googlelogo_grey_tm_color_96x40dp.png" width=3D"96" height=3D"40" alt=
=3D"Google logo"></amp-img></a></td></tr></table></td></tr></table></td></t=
r></table></td></tr></table></div></amp-list></body></html>
--000000000000cd7ce005e9698881
Content-Type: text/html; charset="UTF-8"
Content-Transfer-Encoding: quoted-printable

<html><head></head><body><table style=3D"border-collapse: collapse; width: =
100%; background-color: white; text-align: center;" role=3D"presentation"><=
tr><td style=3D"padding: 24px 0 16px 0;"><table style=3D"border-collapse: c=
ollapse;font-family: Roboto, Arial, Helvetica, sans-serif;hyphens: auto; ov=
erflow-wrap: break-word; word-wrap: break-word; word-break: break-word;disp=
lay: inline-block; width: 90%;max-width: 700px;min-width: 280px; text-align=
: left;" role=3D"presentation"><tr><td style=3D"padding: 0;"><table style=
=3D"width:100%; border: 1px solid #dadce0; border-radius: 8px; border-spaci=
ng: 0; table-layout:fixed; border-collapse: separate;" role=3D"presentation=
"><tr><td style=3D"padding: 4.5%;" dir=3D"ltr"><div style=3D"margin-bottom:=
32px;font-family: Google Sans, Roboto, Arial, Helvetica, sans-serif; font-s=
tyle: normal; font-size: 28px; line-height: 36px; color: #3c4043;">Comfort =
Inyang shared a spreadsheet</div><table style=3D"border-collapse: collapse;=
font-family: Roboto, Arial, Helvetica, sans-serif; font-size:16px; line-hei=
ght:24px; color:#202124; letter-spacing:0.1px; table-layout:fixed; width:10=
0%; overflow-wrap: break-word;" role=3D"presentation"><tr><td style=3D"padd=
ing: 0; vertical-align:top; width:50px;"><!--[if mso]><v:oval xmlns:v=3D"ur=
n:schemas-microsoft-com:vml" xmlns:w=3D"urn:schemas-microsoft-com:office:wo=
rd" style=3D"height:50px;width:50px;" fill=3D"t" stroke=3D"f"><v:fill type=
=3D"frame" src=3D"https://ssl.gstatic.com/s2/profiles/images/silhouette64.p=
ng" alt=3D"Unknown profile photo" style=3D"height:50px;width:50px;"/></v:ov=
al><![endif]--><div style=3D"mso-hide:all;"><img style=3D"border-radius:50%=
; display:block;" width=3D"50" height=3D"50" src=3D"https://ssl.gstatic.com=
/s2/profiles/images/silhouette64.png" alt=3D"Unknown profile photo"></div><=
/td><td style=3D"padding: 0; vertical-align:top; padding-left:12px;"><div s=
tyle=3D"padding-top:12px;">Comfort Inyang (<a href=3D"mailto:icey.me6@gmail=
.com" style=3D"color:inherit;text-decoration:none">icey.me6@gmail.com</a>) =
has invited you to <b>edit</b> the following spreadsheet:</div></td></tr></=
table><table style=3D"border-spacing:0 4px; table-layout:fixed; width:100%;=
 overflow-wrap: break-word;" role=3D"presentation"><tr style=3D"height:28px=
;"></tr><tr><td style=3D"padding: 0;"><a href=3D"https://docs.google.com/sp=
readsheets/d/1icYCsnRoKBvxpYXGCIPwzkUuc9BWlXLkN_Q9R01CwOM/edit?usp=3Dsharin=
g_eil_m&amp;ts=3D632ed817" target=3D"_blank" style=3D"color: #3c4043; displ=
ay: inline-block; max-width: 100%; text-decoration: none; vertical-align: t=
op;border: 1px solid #DADCE0; border-radius: 16px; white-space: nowrap;"><d=
iv style=3D"line-height: 18px; overflow: hidden; text-overflow: ellipsis;pa=
dding: 6px 12px;"><span style=3D"display: inline-block; vertical-align: top=
; min-width: 26px; width: 26px;"><img src=3D"https://ssl.gstatic.com/docs/d=
oclist/images/mediatype/icon_1_spreadsheet_x64.png" width=3D"18" height=3D"=
18" style=3D"vertical-align: top;" role=3D"presentation"></span><span style=
=3D"font: 500 14px/18px Google Sans, Roboto, Arial, Helvetica, sans-serif; =
display: inline; letter-spacing: 0.2px;">1st Principles Coding Mentorship p=
rogram (Responses)</span></div></a></td></tr></table><table style=3D"border=
-collapse: collapse;" role=3D"presentation"><tr style=3D"height: 32px"><td>=
</td></tr></table><div><!--[if mso]><v:roundrect xmlns:v=3D"urn:schemas-mic=
rosoft-com:vml" xmlns:w=3D"urn:schemas-microsoft-com:office:word" href=3D"h=
ttps://docs.google.com/spreadsheets/d/1icYCsnRoKBvxpYXGCIPwzkUuc9BWlXLkN_Q9=
R01CwOM/edit?usp=3Dsharing_eip_m&amp;ts=3D632ed817" style=3D"height:36px;v-=
text-anchor:middle;width:100px;" arcsize=3D"50%" stroke=3D"f" fillcolor=3D"=
#1a73e8"><w:anchorlock/><center style=3D"color:#ffffff;font-family:Arial,He=
lvetica,sans-serif;font-weight:500;font-size:14px;">Open </center></v:round=
rect><![endif]--><a href=3D"https://docs.google.com/spreadsheets/d/1icYCsnR=
oKBvxpYXGCIPwzkUuc9BWlXLkN_Q9R01CwOM/edit?usp=3Dsharing_eip_m&amp;ts=3D632e=
d817" class=3D"material-button material-button-filled" target=3D"_blank" ta=
bindex=3D"0" role=3D"button" style=3D"mso-hide:all;padding: 0 24px;font: 50=
0 14px/36px Google Sans, Roboto, Arial, Helvetica, sans-serif; border: none=
; border-radius: 18px; box-sizing: border-box; display: inline-block; lette=
r-spacing: .25px; min-height: 36px; text-align: center; text-decoration: no=
ne;background-color: #1a73e8; color: #fff; cursor: pointer;">Open</a></div>=
<table style=3D"border-collapse: collapse;" role=3D"presentation"><tr style=
=3D"height: 32px"><td></td></tr></table><div style=3D"font-size: 12px; colo=
r: #5F6368">If you don\'t want to receive files from this person, <a href=3D=
"https://drive.google.com/drive/blockuser?blockerEmail=3Deqliqandfriends@gm=
ail.com&amp;blockeeEmail=3Dicey.me6@gmail.com&amp;usp=3Dsharing_eib_m" targ=
et=3D"_blank" style=3D"color: #1a73e8; text-decoration: none;">block the se=
nder</a> from Drive</div></td></tr></table><table style=3D"border-collapse:=
 collapse; width: 100%;" role=3D"presentation"><tr><td style=3D"padding: 24=
px 4.5%"><table style=3D"border-collapse: collapse; width: 100%;" dir=3D"lt=
r"><tr><td style=3D"padding: 0;font-family: Roboto, Arial, Helvetica, sans-=
serif; color: #5F6368; width: 100%; font-size: 12px; line-height: 16px; min=
-height: 40px; letter-spacing: .3px;">Google LLC, 1600 Amphitheatre Parkway=
, Mountain View, CA 94043, USA<br/> You have received this email because <a=
 href=3D"mailto:icey.me6@gmail.com" style=3D"color:inherit;text-decoration:=
none">icey.me6@gmail.com</a> shared a spreadsheet with you from Google Shee=
ts.</td><td style=3D"padding: 0;padding-left: 20px; min-width: 96px"><a hre=
f=3D"https://www.google.com/" target=3D"_blank"><img src=3D"https://www.gst=
atic.com/images/branding/googlelogo/2x/googlelogo_grey_tm_color_96x40dp.png=
" width=3D"96" height=3D"40" alt=3D"Google logo"></a></td></tr></table></td=
></tr></table></td></tr></table></td></tr></table></body></html>
--000000000000cd7ce005e9698881--')

echo data.body