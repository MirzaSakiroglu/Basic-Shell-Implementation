# Sistem Projesi - Terminal Tabanlı Shell Uygulaması

Bu proje, kullanıcıya basit bir terminal arayüzü sunarak sistem komutlarını çalıştırmasına olanak tanır. `ls`, `cat`, `grep` gibi komutlar desteklenmektedir. Ayrıca pipe (`|`) ve yönlendirme (`>`, `>>`, `<`) özellikleri de mevcuttur.

## Özellikler

- Basit terminal arayüzü
- Pipe desteği: `|`
- Çıktı yönlendirme: `>`, `>>`
- Girdi yönlendirme: `<`
- Hata mesajları ve kontrolü

## Örnek Komutlar

```sh
ls -l | grep txt > dosyalar.txt
cat < girdi.txt | sort >> sirali.txt
```

## Derleme

```sh
make
```

## Çalıştırma

```sh
./multi_user_shell
```

## Dosya Yapısı

- `main.c` – Giriş noktası
- `view.c` – Kullanıcı arayüzü
- `controller.c` – Komut yöneticisi
- `Makefile` – Derleme talimatları


