# [SHOP] Clan Tags (CS2)

Плагин для ядра Shop (pisex), позволяющий игрокам покупать и устанавливать клан-теги
Поддерживает как предустановленные теги, так и возможность создания кастомных тегов игроками

## Функционал
- Покупка прав на установку своего тега
- Покупка готовых тегов
- Сохранение в БД

## Требования
- [Metamod:Source](https://www.sourcemm.net/downloads.php?branch=dev)
- [Shop Core](https://stellarteam.store/resource/cs2-shop-core)
- [Utils](https://github.com/Pisex/cs2-menus/releases/latest)

## Переводы
Добавьте в `translations/shop.phrases.txt`:
```ini
"Tags_NameError"
{
	"en"		"Invalid tag name"
	"ua"		"Некоректна назва тегу"
	"ru"		"Неправильное название тега"
	"chi"		"无效的标签名称"
	"es"		"Nombre de etiqueta no válido"
	"de"		"Ungültiger Tagname"
	"pt"		"Nome de tag inválido"
}

"Tags_tooLongError"
{
	"en"		"Tag name is too long (more than 21 characters)"
	"ua"		"Назва тегу занадто довга (понад 21 символ)"
	"ru"		"Название тега слишком длинное (более 21 символа)"
	"chi"		"标签名称太长（超过21个字符）"
	"es"		"El nombre de la etiqueta es demasiado largo (más de 21 caracteres)"
	"de"		"Der Tagname ist zu lang (mehr als 21 Zeichen)"
	"pt"		"O nome da tag é muito longo (mais de 21 caracteres)"
}

"Tags_tooShortError"
{
	"en"		"Tag name is too short"
	"ua"		"Назва тегу занадто коротка"
	"ru"		"Название тега слишком короткое"
	"chi"		"标签名称太短"
	"es"		"El nombre de la etiqueta es demasiado corto"
	"de"		"Der Tagname ist zu kurz"
	"pt"		"O nome da tag é muito curto"
}

"Tags_customTagSet"
{
	"en"		"Custom tag has been set"
	"ua"		"Користувацький тег встановлено"
	"ru"		"Пользовательский тег установлен"
	"chi"		"已设置自定义标签"
	"es"		"Etiqueta personalizada establecida"
	"de"		"Benutzerdefinierter Tag wurde gesetzt"
	"pt"		"Tag personalizada definida"
}

"Tags_customTagGuide"
{
	"en"		"Enter your clan tag, e.g.: !tag <your tag>"
	"ua"		"Введіть свій клан-тег, наприклад: !tag <ваш тег>"
	"ru"		"Введите свой клан-тег, например: !tag <ваш тег>"
	"chi"		"输入您的氏族标签，例如：!tag <您的标签>"
	"es"		"Introduce tu etiqueta de clan, por ejemplo: !tag <tu etiqueta>"
	"de"		"Gib deinen Clan-Tag ein, z. B.: !tag <dein Tag>"
	"pt"		"Digite sua tag de clã, por exemplo: !tag <sua tag>"
}
```
