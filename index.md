---
layout: page
title: Welcome to my blog ...
permalink: /
---
{% for post in site.posts limit : 5 %}
## [{{ post.title }}]({{ post.url | prepend: site.baseurl }})
### {{ post.date | date: "%b %-d, %Y" }}
 {{ post.excerpt }}
{% endfor %}
