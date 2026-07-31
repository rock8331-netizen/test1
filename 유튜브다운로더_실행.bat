@echo off
chcp 65001 > nul
title YouTube Downloader v2.0
echo YouTube Downloader v2.0 실행 중...
start "" "%~dp0dist\YoutubeDownloader.exe"
