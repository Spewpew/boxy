#### Пример
```boxy i=atlas.png b=atlas.hb e=tiles.h p=TILESPREFIX [sprite1] x=0 y=0 w=100 h=100 [sprite2] x=100 y=0 w=100 h=100 omin=127 omax=255```
### Опции
* h,--help - показать справку
* v,--version - показать версию
* i,--image - вводное изображение (может быть текстурный атлас)
* b,--boxy - файл для сохранения хитбоксов
* e,--enum - файл для сохранения Си-энумераций (порядок спрайтов в файле с хитбоксами)
* p,--prefix - префикс Си-энумераций
* c,--conf - прочитать параметры из файла
* r,--header - файл для сохранения Си-структур(парсинг файла хитбоксов)
* omin,--opacity-min - глобальный верхний предел непрозрачности
* omax,--opacity-max - глобальный нижний предел непрозрачности
* ns,--no-sparse - заполнять нулями пропущенные секции в файле (если файловая система не поддерживает перемещение по пустому файлу)
#### Атрибуты спрайта
* x,y,w,h - позиция спрайта в вводном изображении
* omin,omax - индивидуальные пределы непрозрачности (не обязательно)