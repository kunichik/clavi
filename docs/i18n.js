/** Clavi website — i18n module. Supports: en, uk, es */

const TRANSLATIONS = {
  en: {
    nav_features:   "Features",
    nav_platforms:  "Platforms",
    nav_download:   "Download",

    badge_hero:     "Open source · GPLv3 · No telemetry",
    hero_h1:        "Type in any language.<br><span class=\"accent\">Clavi handles the rest.</span>",
    hero_sub:       "Type <code>cafe</code> — get <code>café</code>. Type <code>naive</code> — get <code>naïve</code>. Smart diacritics, wrong layout, phonetic input — corrected silently, across every app, zero cloud.",
    btn_download:   "Download free",
    btn_github:     "View on GitHub",
    hero_note:      "Linux · macOS · Windows · Android · iOS",

    demo_without:   "Without Clavi",
    demo_with:      "With Clavi",
    demo_bad:       "wrong layout",
    demo_good:      "auto-fixed ✓",

    feat_h2:        "Built for people who type in two languages",
    feat_sub:       "Bilingual professionals. Developers who switch layouts constantly. Anyone tired of retyping.",
    feat1_h:        "Auto-detection",
    feat1_p:        "3-layer engine: dictionary lookup → n-gram statistics → on-device LLM. Detects wrong layout in under 1 ms for 95% of cases. Never guesses on short words.",
    feat2_h:        "Translit mode",
    feat2_p:        "Press Ctrl+T, type Latin phonetically — get Ukrainian Cyrillic. pryvit → привіт. KMU 2010 standard. Works offline, forever.",
    feat3_h:        "Always reversible",
    feat3_p:        "Every correction shows a toast: \"Clavi: switched to Укр → [Ctrl+Z]\". Undo in one key. Full undo stack of 10 actions.",
    feat4_h:        "Clipboard history",
    feat4_p:        "Mobile keyboards hide clipboard. Clavi exposes it as a scrollable strip above the keyboard. Tap to paste any of the last 20 items.",
    feat5_h:        "Smart diacritics",
    feat5_p:        "Type a base letter — get a strip of variants: ã â á à a. One tap, no long-press. Frequency-ordered for PT, DE, FR, NO, ES.",
    feat6_h:        "Text fix",
    feat6_p:        "Catches obvious typos after each space: double space, missing apostrophe, repeated chars. Fix, don't rewrite — never changes your words or tone.",

    mobile_badge:   "Mobile · Android & iOS",
    mobile_h2:      "A full keyboard that understands your language",
    mobile_p:       "On mobile, Clavi is the keyboard. Ukrainian ЙЦУКЕН + English QWERTY in one app. Switch layouts with one tap. Type Latin — get Cyrillic instantly.",
    mobile_f1:      "Ukrainian & English layouts with one-tap switch",
    mobile_f2:      "Transliteration: type pryvit → get привіт",
    mobile_f3:      "Clipboard history strip above keyboard",
    mobile_f4:      "Diacritic variants: no long-press needed",
    mobile_f5:      "Word predictions & translation strip",
    mobile_f6:      "Emoji panel on long-press space",

    privacy_h:      "Your keystrokes never leave your device",
    privacy_p:      "No telemetry. No cloud processing. No accounts required. All detection runs locally — dictionary lookup, n-gram model, and on-device LLM. Open source so you can verify it yourself.",

    plat_h2:        "Every platform. One keyboard habit.",
    plat_linux_p:   "X11 & Wayland. System tray icon. apt install coming soon.",
    plat_mac_p:     "Native daemon. Runs silently in menu bar. Universal binary (Apple Silicon + Intel).",
    plat_win_p:     "System tray, auto-start. Works with any app — browser, Word, Slack.",
    plat_android_p: "Full custom IME. Ukrainian ЙЦУКЕН + English QWERTY. Translit, clipboard, diacritics.",
    plat_ios_p:     "Custom Keyboard Extension. Same features as Android. Clipboard requires Full Access.",
    badge_free:     "Free forever",
    badge_soon:     "Coming soon",
    badge_play:     "Coming to Play Store",
    badge_appstore: "Coming to App Store",

    packs_h2:       "Language packs — all free, community-driven",
    packs_sub:      "Licensed under CC BY-SA 4.0. Contribute your language — millions of speakers benefit.",
    pack_contribute:"+ Add yours",

    cmp_h2:         "How Clavi compares",
    cmp_sub:        "The only tool that fixes wrong layout on desktop automatically — and respects your privacy on every platform.",
    cmp_feature:    "Feature",
    cmp_r1:         "Auto wrong-layout detection (desktop)",
    cmp_r2:         "Works on desktop (Linux / macOS / Windows)",
    cmp_r3:         "Zero cloud — fully local",
    cmp_r4:         "Open source",
    cmp_r5:         "Phonetic translit (Latin → Cyrillic)",
    cmp_r6:         "Clipboard history strip (mobile)",
    cmp_r7:         "Smart diacritics — one tap, no long-press",
    cmp_r8:         "No telemetry, no accounts",
    cmp_note:       "Keyman excels at rare-script coverage (2,500+ languages). Gboard and SwiftKey are polished for everyday users but send data to the cloud. Clavi is the only option combining desktop auto-detection, privacy, and modern bilingual UX in one tool.",

    dl_h2:          "Get Clavi",
    dl_desktop_h:   "🐧 Linux / macOS / Windows",
    dl_desktop_p:   "Build from source — CMake 3.25+ and a C++20 compiler.",
    dl_desktop_btn: "Releases on GitHub",
    dl_android_h:   "🤖 Android",
    dl_android_p:   "APK coming soon to Google Play. For now — build from source.",
    dl_android_btn: "Google Play — coming soon",
    dl_ios_h:       " iOS",
    dl_ios_p:       "App Store submission in progress. Build with Xcode 15+.",
    dl_ios_btn:     "App Store — coming soon",

    footer_tagline: "from Latin clavis = key",
    footer_license: "GPLv3",
    footer_issues:  "Issues",
  },

  uk: {
    nav_features:   "Можливості",
    nav_platforms:  "Платформи",
    nav_download:   "Завантажити",

    badge_hero:     "Відкритий код · GPLv3 · Без телеметрії",
    hero_h1:        "Пишіть будь-якою мовою.<br><span class=\"accent\">Clavi зробить решту.</span>",
    hero_sub:       "Пишете <code>ghbdsn</code> — отримуєте <code class=\"uk\">привіт</code>. Пишете <code>pryvit</code> — теж <code class=\"uk\">привіт</code>. Неправильна розкладка, фонетичний ввід, діакритики — виправляється тихо, у будь-якому застосунку, без хмари.",
    btn_download:   "Завантажити безкоштовно",
    btn_github:     "GitHub",
    hero_note:      "Linux · macOS · Windows · Android · iOS",

    demo_without:   "Без Clavi",
    demo_with:      "З Clavi",
    demo_bad:       "неправильна розкладка",
    demo_good:      "автовиправлено ✓",

    feat_h2:        "Для тих, хто пише двома мовами",
    feat_sub:       "Двомовні фахівці. Розробники, які постійно перемикають розкладку. Всі, кому набридло передруковувати.",
    feat1_h:        "Автовизначення",
    feat1_p:        "3-рівневий рушій: словниковий пошук → n-gram статистика → локальна LLM. Визначає неправильну розкладку за &lt;1 мс у 95% випадків. Ніколи не гадає на коротких словах.",
    feat2_h:        "Режим транслітерації",
    feat2_p:        "Натисніть Ctrl+T, пишіть фонетично латиницею — отримуєте українську кирилицю. pryvit → привіт. Стандарт КМУ 2010. Працює офлайн, назавжди.",
    feat3_h:        "Завжди зворотньо",
    feat3_p:        "Кожна корекція показує тост: «Clavi: перемкнуто на Укр → [Ctrl+Z]». Скасування одним натисканням. Стек з 10 дій.",
    feat4_h:        "Історія буфера обміну",
    feat4_p:        "Мобільні клавіатури ховають буфер. Clavi показує його як смугу прокрутки над клавіатурою. Натисніть — вставте будь-який з 20 останніх елементів.",
    feat5_h:        "Розумні діакритики",
    feat5_p:        "Введіть базову літеру — отримайте варіанти: ã â á à a. Один дотик, без довгого натискання. Впорядковані за частотою для PT, DE, FR, NO, ES.",
    feat6_h:        "Виправлення тексту",
    feat6_p:        "Знаходить очевидні помилки після кожного пробілу: подвійний пробіл, пропущений апостроф, повторювані символи. Виправляй, не переписуй — ніколи не змінює ваш стиль.",

    mobile_badge:   "Мобільні · Android та iOS",
    mobile_h2:      "Повноцінна клавіатура, яка розуміє вашу мову",
    mobile_p:       "На мобільному Clavi — це і є клавіатура. Українська ЙЦУКЕН + англійська QWERTY в одному застосунку. Перемикання розкладки одним дотиком.",
    mobile_f1:      "Українська та англійська розкладки з перемиканням одним дотиком",
    mobile_f2:      "Транслітерація: пишіть pryvit → отримуєте привіт",
    mobile_f3:      "Смуга історії буфера обміну над клавіатурою",
    mobile_f4:      "Варіанти діакритиків без довгого натискання",
    mobile_f5:      "Передбачення слів і смуга перекладу",
    mobile_f6:      "Панель емоджі при довгому натисканні пробілу",

    privacy_h:      "Ваші натискання ніколи не залишають пристрій",
    privacy_p:      "Без телеметрії. Без хмарної обробки. Без акаунтів. Усе визначення відбувається локально — словниковий пошук, n-gram модель та локальна LLM. Відкритий код — перевірте самі.",

    plat_h2:        "Кожна платформа. Одна звичка.",
    plat_linux_p:   "X11 та Wayland. Іконка в треї. apt install — незабаром.",
    plat_mac_p:     "Нативний демон. Працює в рядку меню. Universal binary (Apple Silicon + Intel).",
    plat_win_p:     "Іконка в треї, автозапуск. Працює з будь-яким застосунком — браузер, Word, Slack.",
    plat_android_p: "Повноцінна IME. Українська ЙЦУКЕН + англійська QWERTY. Транслітерація, буфер, діакритики.",
    plat_ios_p:     "Custom Keyboard Extension. Ті самі можливості, що на Android. Буфер обміну потребує Full Access.",
    badge_free:     "Безкоштовно назавжди",
    badge_soon:     "Незабаром",
    badge_play:     "Незабаром у Play Store",
    badge_appstore: "Незабаром в App Store",

    packs_h2:       "Мовні паки — всі безкоштовні, спільнота",
    packs_sub:      "Ліцензія CC BY-SA 4.0. Додайте свою мову — мільйони носіїв отримають підтримку.",
    pack_contribute:"+ Додати свою",

    cmp_h2:         "Порівняння з альтернативами",
    cmp_sub:        "Єдиний інструмент, який автоматично виправляє неправильну розкладку на десктопі — і поважає вашу приватність на кожній платформі.",
    cmp_feature:    "Функція",
    cmp_r1:         "Автовиправлення неправильної розкладки (десктоп)",
    cmp_r2:         "Підтримка десктопу (Linux / macOS / Windows)",
    cmp_r3:         "Без хмари — повністю локально",
    cmp_r4:         "Відкритий код",
    cmp_r5:         "Фонетична транслітерація (латиниця → кирилиця)",
    cmp_r6:         "Смуга історії буфера обміну (мобільний)",
    cmp_r7:         "Розумні діакритики — один дотик, без довгого натискання",
    cmp_r8:         "Без телеметрії, без акаунтів",
    cmp_note:       "Keyman охоплює 2500+ мов — ідеально для рідкісних шрифтів. Gboard і SwiftKey зручні, але відправляють дані в хмару. Clavi — єдиний варіант, що поєднує автовизначення розкладки на десктопі, приватність і сучасний двомовний UX в одному інструменті.",

    dl_h2:          "Отримати Clavi",
    dl_desktop_h:   "🐧 Linux / macOS / Windows",
    dl_desktop_p:   "Зберіть з вихідного коду — CMake 3.25+ та C++20 компілятор.",
    dl_desktop_btn: "Релізи на GitHub",
    dl_android_h:   "🤖 Android",
    dl_android_p:   "APK незабаром у Google Play. Поки — зберіть з вихідного коду.",
    dl_android_btn: "Google Play — незабаром",
    dl_ios_h:       " iOS",
    dl_ios_p:       "Подача до App Store в процесі. Зберіть у Xcode 15+.",
    dl_ios_btn:     "App Store — незабаром",

    footer_tagline: "від латинського clavis = ключ",
    footer_license: "GPLv3",
    footer_issues:  "Проблеми",
  },

  es: {
    nav_features:   "Funciones",
    nav_platforms:  "Plataformas",
    nav_download:   "Descargar",

    badge_hero:     "Código abierto · GPLv3 · Sin telemetría",
    hero_h1:        "Escribe en cualquier idioma.<br><span class=\"accent\">Clavi se encarga del resto.</span>",
    hero_sub:       "Escribes <code>manana</code> — obtienes <code>mañana</code>. Escribes <code>nino</code> — claro, <code>niño</code>. Diacríticos inteligentes — corregidos en silencio, en cualquier app, sin nube.",
    btn_download:   "Descargar gratis",
    btn_github:     "Ver en GitHub",
    hero_note:      "Linux · macOS · Windows · Android · iOS",

    demo_without:   "Sin Clavi",
    demo_with:      "Con Clavi",
    demo_bad:       "teclado incorrecto",
    demo_good:      "corregido automáticamente ✓",

    feat_h2:        "Para quienes escriben en dos idiomas",
    feat_sub:       "Profesionales bilingües. Desarrolladores que cambian de distribución constantemente. Todos los que están hartos de reescribir.",
    feat1_h:        "Detección automática",
    feat1_p:        "Motor de 3 capas: búsqueda en diccionario → estadísticas n-gram → LLM local. Detecta el teclado incorrecto en &lt;1 ms en el 95% de los casos.",
    feat2_h:        "Modo transliteración",
    feat2_p:        "Presiona Ctrl+T, escribe fonéticamente en latín — obtienes cirílico ucraniano. pryvit → привіт. Estándar KMU 2010. Funciona sin conexión, siempre.",
    feat3_h:        "Siempre reversible",
    feat3_p:        "Cada corrección muestra una notificación: «Clavi: cambió a Укр → [Ctrl+Z]». Deshacer con una tecla. Pila de 10 acciones.",
    feat4_h:        "Historial del portapapeles",
    feat4_p:        "Los teclados móviles ocultan el portapapeles. Clavi lo muestra como una barra deslizable encima del teclado. Toca para pegar cualquiera de los últimos 20 elementos.",
    feat5_h:        "Diacríticos inteligentes",
    feat5_p:        "Escribe una letra base — obtén variantes: ã â á à a. Un toque, sin mantener presionado. Ordenadas por frecuencia para PT, DE, FR, NO, ES.",
    feat6_h:        "Corrección de texto",
    feat6_p:        "Detecta errores obvios tras cada espacio: doble espacio, apóstrofe faltante, caracteres repetidos. Corrige, no reescribe — nunca cambia tu estilo.",

    mobile_badge:   "Móvil · Android e iOS",
    mobile_h2:      "Un teclado completo que entiende tu idioma",
    mobile_p:       "En móvil, Clavi es el teclado. Ucraniano ЙЦУКЕН + inglés QWERTY en una sola app. Cambia de distribución con un toque.",
    mobile_f1:      "Distribuciones ucraniana e inglesa con cambio en un toque",
    mobile_f2:      "Transliteración: escribe pryvit → obtén привіт",
    mobile_f3:      "Barra de historial del portapapeles sobre el teclado",
    mobile_f4:      "Variantes de diacríticos sin mantener presionado",
    mobile_f5:      "Predicciones de palabras y barra de traducción",
    mobile_f6:      "Panel de emoji con pulsación larga en el espacio",

    privacy_h:      "Tus pulsaciones nunca salen de tu dispositivo",
    privacy_p:      "Sin telemetría. Sin procesamiento en la nube. Sin cuentas requeridas. Toda la detección se realiza localmente. Código abierto para que puedas verificarlo tú mismo.",

    plat_h2:        "Cada plataforma. Un solo hábito.",
    plat_linux_p:   "X11 y Wayland. Icono en la bandeja del sistema. apt install próximamente.",
    plat_mac_p:     "Daemon nativo. Se ejecuta en la barra de menú. Binario universal (Apple Silicon + Intel).",
    plat_win_p:     "Bandeja del sistema, inicio automático. Funciona con cualquier app — navegador, Word, Slack.",
    plat_android_p: "IME personalizada completa. Ucraniano ЙЦУКЕН + inglés QWERTY. Translit, portapapeles, diacríticos.",
    plat_ios_p:     "Extensión de teclado personalizada. Mismas funciones que en Android. El portapapeles requiere acceso completo.",
    badge_free:     "Gratis para siempre",
    badge_soon:     "Próximamente",
    badge_play:     "Próximamente en Play Store",
    badge_appstore: "Próximamente en App Store",

    packs_h2:       "Paquetes de idiomas — todos gratis, impulsados por la comunidad",
    packs_sub:      "Licencia CC BY-SA 4.0. Contribuye tu idioma — millones de hablantes se beneficiarán.",
    pack_contribute:"+ Agregar el tuyo",

    cmp_h2:         "Cómo se compara Clavi",
    cmp_sub:        "La única herramienta que corrige automáticamente la distribución incorrecta en escritorio — y respeta tu privacidad en cada plataforma.",
    cmp_feature:    "Función",
    cmp_r1:         "Detección automática de distribución incorrecta (escritorio)",
    cmp_r2:         "Compatible con escritorio (Linux / macOS / Windows)",
    cmp_r3:         "Sin nube — completamente local",
    cmp_r4:         "Código abierto",
    cmp_r5:         "Transliteración fonética (latín → cirílico)",
    cmp_r6:         "Historial del portapapeles (móvil)",
    cmp_r7:         "Diacríticos inteligentes — un toque, sin mantener presionado",
    cmp_r8:         "Sin telemetría, sin cuentas",
    cmp_note:       "Keyman cubre más de 2.500 idiomas — ideal para escrituras raras. Gboard y SwiftKey son convenientes pero envían datos a la nube. Clavi es la única opción que combina detección automática en escritorio, privacidad y UX bilingüe moderno en una sola herramienta.",

    dl_h2:          "Obtener Clavi",
    dl_desktop_h:   "🐧 Linux / macOS / Windows",
    dl_desktop_p:   "Compila desde el código fuente — CMake 3.25+ y un compilador C++20.",
    dl_desktop_btn: "Versiones en GitHub",
    dl_android_h:   "🤖 Android",
    dl_android_p:   "APK próximamente en Google Play. Por ahora — compila desde el código fuente.",
    dl_android_btn: "Google Play — próximamente",
    dl_ios_h:       " iOS",
    dl_ios_p:       "Envío a App Store en curso. Compila con Xcode 15+.",
    dl_ios_btn:     "App Store — próximamente",

    footer_tagline: "del latín clavis = llave",
    footer_license: "GPLv3",
    footer_issues:  "Problemas",
  },
};

const LANG_LABELS = { en: "EN", uk: "УК", es: "ES" };

function getLang() {
  const stored = localStorage.getItem("clavi_lang");
  if (stored && TRANSLATIONS[stored]) return stored;
  const browser = navigator.language.slice(0, 2).toLowerCase();
  return TRANSLATIONS[browser] ? browser : "en";
}

function applyLang(lang) {
  const t = TRANSLATIONS[lang];
  document.querySelectorAll("[data-i18n]").forEach(el => {
    const key = el.dataset.i18n;
    if (t[key] !== undefined) el.innerHTML = t[key];
  });
  document.documentElement.lang = lang;
  localStorage.setItem("clavi_lang", lang);

  // Update switcher buttons
  document.querySelectorAll(".lang-btn").forEach(btn => {
    btn.classList.toggle("active", btn.dataset.lang === lang);
  });
}

function buildSwitcher() {
  const nav = document.querySelector(".nav-links");
  const wrap = document.createElement("div");
  wrap.className = "lang-switcher";
  Object.keys(LANG_LABELS).forEach(lang => {
    const btn = document.createElement("button");
    btn.className = "lang-btn";
    btn.dataset.lang = lang;
    btn.textContent = LANG_LABELS[lang];
    btn.addEventListener("click", () => applyLang(lang));
    wrap.appendChild(btn);
  });
  nav.insertBefore(wrap, nav.querySelector(".btn-nav"));
  return wrap;
}

document.addEventListener("DOMContentLoaded", () => {
  buildSwitcher();
  applyLang(getLang());
});
