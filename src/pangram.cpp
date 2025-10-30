#include "pangram.hpp"

const char* getPreviewTextForLanguage(std::string language) {
    if (language == "Jpan") {
        return "人類社会のすべての構成員の固有の尊厳と平等で譲ることのできない権利とを承認することは、世界における自由、正義及び平和の基礎であるので、";
    } else if (language == "Phag") {
        return "ꡛ ꡘꡧꡠ ᠂ ꡏꡖ ꡋ ꡓꡖꡜ ᠂ ꡛꡧ ꡈꡋ ꡈꡱꡖꡜ ᠂ ꡛ ꡏꡟꡈ ꡌꡋ ꡋꡖꡜ ᠂ ꡓꡘ ꡈꡋ ꡈꡠ ᠂ ꡝ ꡌꡞ ᠂ ꡐ, ᠂ ꡂꡖꡡ ꡘ ꡓ ꡊꡱꡞ ꡚꡖ ᠂ ꡝ ꡊꡜꡞ ꡀꡖ ꡘ ꡊꡱꡞ ꡚꡖ ᠂ ꡐ ᠂ ꡛ ꡏꡖ ꡋꡖꡜ ᠂ ꡠ ꡓ ᠂ ꡓꡘ ꡈꡋ ꡈꡠ꡶ ᠂ ꡠ ꡈꡠ ᠂ ꡛ ꡘꡧꡠ ᠂ ꡐꡠ ꡈ ꡋꡖꡈꡘ ꡀꡚꡀ ꡈꡞ ꡳꡎꡜꡨꡖ ᠂ ꡛꡟ ꡛꡏ ꡌꡋ ꡋꡖꡜ ᠂ ꡛꡋ ꡈꡞ꡶ ᠂ ꡝ ꡌꡞ ᠂ ꡐ, ᠂ ꡛ ꡘꡧꡠ ꡌꡞ ᠂ ꡎꡋ ꡊꡜꡟ ꡈꡧꡎꡜꡖ ꡓ ꡋ ꡗꡖ ᠂ ꡌ ꡘꡛ ꡌ ꡳꡘ ᠂ ꡓꡨ ꡓ ꡜ ꡘꡋ ꡈꡟ꡶";
    } else if (language == "Deva") {
        return "Ang lahat ng tao'y isinilang na malaya at pantay-pantay sa karangalan at mga karapatan. Sila'y pinagkalooban ng katwiran at budhi at dapat magpalagayan ang isa't isa sa diwa ng pagkakapatiran.";
    } else if (language == "Arab") {
        return "لا يعرض أحد لتدخل تعسفي في حياته الخاصة أو أسرته أو مسكنه أو مراسلاته أو لحملات على شرفه وسمعته. ولكل شخص الحق في حماية القانون من مثل هذا التدخل أو تلك الحملات.";
    } else if (language == "Hebr") {
        return "כל אדם זכאי לחירות הדעה והבטוי, לרבות החירות להחיק בדעות ללא כל הפרעה, ולבקש ידיעות ודעות, ולקבלן ולמסרן בכל הדרכים וללא סייגי גבולות";
    } else if (language == "Kore") {
        return "이 선언의 어떠한 규정도 어떤 국가, 집단 또는 개인에게 이 선언에 규정된 어떠한 권리와 자유를 파괴하기 위한 활동에 가담하거나 또는 행위를 할 수 있는 권리가 있는 것으로 해석되어서는 아니된다.";
    } else if (language == "Hans") {
        return "鉴于对人类家庭所有成员的固有尊严及其平等的和不移的权利的承认,乃是世界自由、正义与和平的基础,";
    } else if (language == "Hant") {
        return "鑑於對人類家庭所有成員的固有尊嚴及其平等的和不移的權利的承認，乃是世界自由、正義與和平的基礎，";
    } else if (language == "Knda") {
        return "Ang lahat ng tao'y isinilang na malaya at pantay-pantay sa karangalan at mga karapatan. Sila'y pinagkalooban ng katwiran at budhi at dapat magpalagayan ang isa't isa sa diwa ng pagkakapatiran.";
    } else if (language == "Beng") {
        return "শাসনতন্ত্রে বা আ‌ইনে প্রদত্ত মৌলিক অধিকার লঙ্ঘনের ক্ষেত্রে উপযুক্ত জাতীয় বিচার আদালতের কাছ থেকে কার্যকর প্রতিকার লাভের অধিকার প্রত্যেকের‌ই রয়েছে।";
    } else if (language == "Telu") {
        return "ప్రతి వ్యక్తికిని అభిప్రాయస్వాతంత్ర్యమునకును, భావ ప్రకటన స్వాతంత్ర్యమునకును, హక్కు గలదు. పరుల జోక్యము లేక, స్వాభిప్రాయమును గలిగియుండుటకు స్వాతంత్ర్యమును, రాజ్యసీమానిరపేక్షముగా, నెట్టి మధ్యస్థ మార్గముననైన సమాచార, సంసూచనలను అన్వేషించుటకు, పొందుటకు, ఉపపాదించుటకు, స్వాతంత్ర్యమును ఈ హక్కులో నిమిదియున్నవి.";
    } else if (language == "Laoo") {
        return "ດ້ວຍເຫດວ່າ: ການຮັບຮູ້ກຽດຕິສັກອັນມີປະຈຳຢູ່ຕົວບຸກຄົນໃນວົງສະກຸນຂອງມະນຸດທຸກໆຄົນ ແລະ ການຮັບຮູ້ສິດສະເໝີພາບ ແລະ ສະເຖຍລະພາບຂອງບຸກຄົນເຫຼົ່ານັ້ນ ປະກອບເປັນຮາກຖານຂອງສິດເສລີພາບ ຍຸດຕິທຳ ແລະ ສັນຕິພາບຂອງໂລກ.";
    } else if (language == "Taml") {
        return "கருத்துச் சுதந்திரத்துக்கும் பேச்சுச் சுதந்திரத்துக்கும் எவருக்கும் உரிமையுண்டு. இவ்வுரிமையானது தலையீடின்றிக் கருத்துகளைக் கொண்டிருத்தற்கும், எவ்வூடகம் மூலமும் எல்லைகளைப் பொருட்படுத்தாமலும் தகவலையும் கருத்துக்களையும் நாடுவதற்கும் பெறுவதற்கும் பரப்புவதற்குமான சுதந்திரத்தையும் உள்ளடக்கும்.";
    } else if (language == "Mand") {
        return "ࡕࡉࡁࡉࡋ ࡓࡌࡀ ࡀࡐࡀࡓࡀ ࡀࡎࡀࡓ ࡏࡅࡕࡓࡀ ࡂࡁࡓࡀ ࡍࡄࡅࡓࡀ ࡁࡓࡀࡄࡉࡌ ࡉࡀࡄࡅࡃࡉࡋࡀࡏࡉࡋ ࡓࡁࡉࡀ ࡀࡃࡌ ࡐࡀࡂࡓࡀ ࡁࡉࡋࡀ ࡊࡋࡀࡕࡀ ࡍࡉࡔࡉࡌࡕࡀ ࡀࡐࡀ ࡌࡀࡍࡃࡀ ࡖࡍࡄࡅࡓࡀ ࡄࡀࡁࡔࡀࡁࡀ ࡌࡉࡔࡀ ࡕࡀࡓࡌࡉࡃࡀ ࡊࡀࡄࡍࡀ ࡌࡀࡍࡀ ࡊࡋࡀࡕ࡙ࡀ ࡄࡉࡉࡀ ࡕࡓࡉࡎࡀࡓ ࡁࡀࡁࡉࡋ ࡓࡁࡀ ࡂࡉࡍࡆࡀ ࡔࡅࡌ ࡒࡀࡃࡅࡔࡀࡍ ࡌࡀࡓࡀ ࡀࡍࡀࡔࡀ ࡀࡎࡓࡀ ࡁࡉࡋࡀࡅࡓࡀ ࡁࡓ ࡓࡀࡁࡅࡕࡀ ࡌࡀࡑࡁࡅࡕࡀ ࡌࡀࡍࡃࡉࡉࡀࡍࡀ ࡁࡉࡕ ࡆࡀࡌࡀࡍ ࡍࡉࡎࡓࡀࡕ";
    } else if (language == "Tavt") {
        return "ꪋꪴ ꫛ ꪝꪮꪣ ꪼꪒ ꪣꪲ ꪁꪫꪸꪙ ꪈꪾ ꪜꪴꪙ ꪮꪮꪀ ꪒꪰꪀ ꪵꪒꪣ ꪀꪾꪚ ꪎꪱꪉ ꪭꪲꪒ ꪅꪮꪉ, ꪹꪕꪸꪉ ꪀꪱ ꪕꪳ ꪕꪱꪉ ꪜꪸꪙ ꪹꪋ ꪎꪱꪥ ꪵꪀꪉ ꪭꪲꪒ ꪅꪮꪉ, ꪀꪾꪚ ꪼꪒ ꪕꪳ ꪕꪱꪉ ꪮꪮꪀ ꪘꪱ ꫛ ꪹꪋ ꪎꪱꪥ ꪣꪲ ꪭꪳ ꪎꪱꪉ ꪭꪲꪒ ꪅꪮꪉ ꪵꪔ ꪜꪱꪉ ꪒꪱꪥ ꪋꪸꪙ ꪣꪱ ꪹꪤꪸꪒ ꪹꪎꪸꪙ ꪡꪮꪙ.";
    } else if (language == "Thai") {
        return "โดยที่การยอมรับศักดิ์ศรีแต่กำเนิด และสิทธิที่เท่าเทียมกันและที่ไม่อาจเพิกถอนได้ของสมาชิกทั้งมวลแห่งครอบครัวมนุษยชาติ เป็นพื้นฐานแห่งอิสรภาพ ความยุติธรรม และสันติภาพในโลก.";
    } else if (language == "Talu") {
        return "ᦝᧂᦑᦸᦰ ᦍᦸᧆᦑᦲᧈᦷᦢᦆᧄ ᦅᧀᦂᦱᧂᦐᦸᧂ ᦂᦱᧁ ᦙᦸᧃᦟᦱᧆᦓᧄᧉ, ᦶᦙᧈᦷᦎᦶᦂᧄᧉ ᦃᦸᧂᧈ ᦋᧄᧉ ᦶᦀᧁ ᦵᦉᧄᧉ ᦺᦜᧈ ᦺᦃ ᦋᦻᦵᦣᧀ, ᦔᦱᧂ ᦓᦲᦰ ᦋᦻ ᦈᧅ ᦺᦃ ᦺᦔ ᦡᦽᧉᦓᦲᧉ ᦑᦱᧃᦜᧂ ᦶᦎᧈ ᦂᦸᧃᧈ, ᦈᦱ ᦉᦸᧃᧈ ᦠᦹᧉ ᦵᦗᦲᧃᧈᦣᦼᧉᦡᦽᧉᦆᦱᧁᧈᦟᧂᦂᦱ.";
    } else if (language == "Khoj") {
        return "𑈩𑈤𑈨𑈦𑈬𑈺𑈀𑈞𑈩𑈬𑈞𑈺𑈁𑈐𑈶𑈬𑈛𑈺𑈂𑈺𑈀𑈐𑈶𑈙𑈺𑈂𑈺𑈪𑈈𑈶𑈞𑈺𑈑𑈥𑈺𑈪𑈨𑈬𑈧𑈥𑈺𑈈𑈀𑈞𑈺𑈞𑈦𑈬𑈞𑈦𑈺𑈟𑈥𑈛𑈬𑈔𑈥𑈬𑈺𑈁𑈪𑈞𑈸𑈺𑈀𑈞𑈪𑈞𑈺𑈈𑈥𑈺𑈀𑈈𑈶𑈧𑈺𑈂𑈺𑈐𑈶𑈤𑈥𑈦𑈺𑈪𑈬𑈩𑈧𑈺𑈔𑈥𑈨𑈺𑈁𑈪𑈥,𑈺𑈈𑈦𑈥𑈺𑈀𑈞𑈪𑈞𑈺𑈈𑈥𑈺𑈪𑈈𑈺𑈢𑈥𑈥𑈺𑈩𑈧𑈞𑈺𑈣𑈧𑈥𑈥𑈏𑈀𑈦𑈺𑈥𑈺𑈨𑈬𑈦𑈨𑈺𑈩𑈬𑈨𑈈𑈺𑈀𑈉𑈶𑈙𑈥𑈬𑈦𑈺𑈈𑈦𑈘𑈺𑈊𑈪𑈦𑈺𑈐𑈥𑈸";
    } else if (language == "Cyrl") {
        return "Вси́ бо лю́дїе родѧ́тсѧ свобо́дни и҆ ра́вни въ досто́инствѣ и҆ зако́нѣ. Ѻ҆нѝ сꙋ́ть ѡ҆дарова́ни ра́зꙋмомъ и҆ со́вѣстїю и҆ до́лжни сꙋ́ть дѣ́ѧти въ дꙋ́сѣ бра́тства.";
    } else if (language == "Sinh") {
        return "සෑම අවස්ථාවකදී ම නීතිය ඉදිරියේ පුද්ගලයකු වශයෙන් පිළිගනු ලැබීමේ අයිතිය සෑම කෙනෙකුම සතුය. සියල්ලන්ම නීතිය ඉදිරියෙහි එකිනෙකාට සමාන වන අතර, නීතීයේ ආරක්ෂාව ලැබීමට කිසිදු විශේෂයක් නොමැතිව.";
    } else if (language == "en_Latn" || language == "Latn") {
        return "Everyone has the right to freedom of thought, conscience and religion; this right includes freedom to change his religion or belief, and freedom, either alone or in community with others and in public or private, to manifest his religion or belief in teaching, practice, worship and observance.";
    } else if (language == "es_Latn") {
        return "Considerando que la libertad, la justicia y la paz en el mundo tienen por base el reconocimiento de la dignidad intrínseca y de los derechos iguales e inalienables de todos los miembros de la familia humana";
    } else if (language == "de_Latn") {
        return "Alle Menschen sind frei und gleich an Würde und Rechten geboren. Sie sind mit Vernunft und Gewissen begabt und sollen einander im Geist der Brüderlichkeit begegnen.";
    } else if (language == "vi_Latn") {
        return "Mọi người đều có quyền được hưởng trật tự xã hội và trật tự quốc tế trong đó các quyền và tự do nêu trong Bản tuyên ngôn này có thể được thực hiện đầy đủ.";
    } else if (language == "sk_Latn") {
        return "Každý má právo na slobodu myslenia, svedomia a náboženstva; toto právo obsahuje aj voľnosť zmeniť náboženstvo alebo vieru, ako i slobodu prejavovať svoje náboženstvo alebo vieru, sám alebo společne s inými, či už verejne alebo súkromne, vyučovaním, vykonávaním náboženských úkonov, bohoslužbou a zachovávaním obradov.";
    } else if (language == "nl_Latn") {
        return "Overwegende, dat erkenning van de inherente waardigheid en van de gelijke en onvervreemdbare rechten van alle leden van de mensengemeenschap grondslag is voor de vrijheid, gerechtigheid en vrede in de wereld;";
    } else if (language == "hr_Latn") {
        return "Nitko ne smije biti podvrgnut mučenju ili okrutnom, nečovječnom ili ponižavajućem postupku ili kažnjavanju.";
    } else if (language == "fr_Latn") {
        return "Considérant que la reconnaissance de la dignité inhérente à tous les membres de la famille humaine et de leurs droits égaux et inaliénables constitue le fondement de la liberté, de la justice et de la paix dans le monde,";
    } else if (language == "hu_Latn") {
        return "Senkinek magánéletébe, családi ügyeibe, lakóhelye megválasztásába vagy levelezésébe nem szabad önkényesen beavatkozni, sem pedig becsületében vagy jó hírnevében megsérteni. Minden személynek joga van az ilyen beavatkozásokkal vagy sértésekkel szemben a törvény védelméhez.";
    } else if (language == "pt_Latn") {
        return "Todos os seres humanos nascem livres e iguais em dignidade e direitos. São dotados de razão e consciência e devem agir em relação uns aos outros com espírito de fraternidade.";
    } else if (language == "da_Latn") {
        return "Da anerkendelse af den mennesket iboende værdighed og af de lige og ufortabelige rettigheder for alle medlemmer af den menneskelige familie er grundlaget for frihed, retfærdighed og fred i verden,";
    } else if (language == "it_Latn") {
        return "Considerato che il riconoscimento della dignità inerente a tutti i membri della famiglia umana e dei loro diritti, uguali ed inalienabili, costituisce il fondamento della libertà, della giustizia e della pace nel mondo;";
    } else if (language == "is_Latn") {
        return "Það ber að viðurkenna, að hver maður sé jafnborinn til virðingar og réttinda, er eigi verði af honum tekin, og er þetta undirstaða frelsis, réttlætis og friðar i heiminum.";
    } else if (language == "ro_Latn") {
        return "Considerând că recunoașterea demnității inerente tuturor membrilor familiei umane și a drepturilor lor egale și inalienabile constituie fundamentul libertății, dreptății și păcii în lume.";
    } else if (language == "emoji") {
        return "🥰💀✌︎🌴🐢🐐🍄⚽🍻👑📸😬👀🚨🏡🐦‍🔥🍋‍🟩🍄‍🟫🙂‍↕︎🕊︎🏆😻🌟🧿🍀🎨🍜";
    } else if (language == "symbols") {
        return "⛾⛿☯☸ ⛩⛰⛱⛴⛷⛸⛹ ♸⚥☊☍☓☤ 🄰🄱🆈🆉 ⚖♇♪♬";
    } else if (language == "symbols2") {
        return "🌍✄✎ 🏔🏕🏌🏍🎭🎮 🯅🯆🯇🯉 🡢🡭🡱🡼 🯱🯲🯳🯴🯵🯶 🂮🂱🂲🂳";
    } else if (language == "music") {
        return "𝄆 𝄙𝆏 𝅗𝅘𝅥𝅘𝅥𝅯𝅘𝅥𝅱 𝄞𝄟𝄢 𝄾𝄿𝄎 𝄴 𝄶𝅁 𝄭𝄰 𝇛𝇜 𝄊 𝄇 𝀸𝀹𝀺𝀻𝀼𝀽 𝈀𝈁𝈂𝈃𝈄𝈅";
    } else {
        return "The five boxing wizards jump quickly.";
    }
}
