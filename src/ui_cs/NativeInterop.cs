using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace YoutubeDownloader
{
    /// <summary>
    /// C++ YoutubeCore.dll P/Invoke 연동 모듈
    /// DLL 경로: 실행파일 위치 기준 ../../build/YoutubeCore.dll
    /// </summary>
    internal static class NativeInterop
    {
        // DLL 경로 탐색: 실행파일 동일 폴더 -> build 폴더 순
        private static string GetDllPath()
        {
            string baseDir = AppDomain.CurrentDomain.BaseDirectory;
            string[] candidates = {
                Path.Combine(baseDir, "YoutubeCore.dll"),
                Path.GetFullPath(Path.Combine(baseDir, "..", "build", "YoutubeCore.dll")),
                Path.GetFullPath(Path.Combine(baseDir, "..", "..", "build", "YoutubeCore.dll")),
                Path.GetFullPath(Path.Combine(baseDir, "..", "..", "..", "build", "YoutubeCore.dll")),
                Path.GetFullPath(Path.Combine(baseDir, "..", "..", "..", "..", "build", "YoutubeCore.dll"))
            };
            foreach (string p in candidates)
            {
                if (File.Exists(p)) return p;
            }

            // Extract embedded YoutubeCore.dll for standalone Single-File execution
            string tempDir = Path.Combine(Path.GetTempPath(), "YoutubeDownloader");
            Directory.CreateDirectory(tempDir);
            string tempDllPath = Path.Combine(tempDir, "YoutubeCore.dll");
            try
            {
                var assembly = System.Reflection.Assembly.GetExecutingAssembly();
                using Stream? stream = assembly.GetManifestResourceStream("YoutubeDownloader.Resources.YoutubeCore.dll")
                                     ?? assembly.GetManifestResourceStream("YoutubeDownloader.YoutubeCore.dll");
                if (stream != null)
                {
                    using FileStream fs = new FileStream(tempDllPath, FileMode.Create, FileAccess.Write);
                    stream.CopyTo(fs);
                    return tempDllPath;
                }
            }
            catch { }

            return candidates[0];
        }

        private static readonly string DllPath = GetDllPath();

        private static IntPtr _dll = IntPtr.Zero;

        // ── 함수 포인터 델리게이트 선언 ──────────────────────
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int  FindFFmpegPathDelegate([Out] byte[] outPath, int bufSize);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void FormatBytesDelegate(long bytes, [Out] byte[] outStr, int bufSize);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int  KillProcessByIdDelegate(int pid);

        // ── 함수 포인터 인스턴스 ──────────────────────────────
        private static FindFFmpegPathDelegate?  _findFFmpeg;
        private static FormatBytesDelegate?     _formatBytes;
        private static KillProcessByIdDelegate? _killProcess;

        // ── P/Invoke: LoadLibrary / GetProcAddress ──────────
        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true)]
        private static extern IntPtr LoadLibrary(string lpFileName);

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32.dll")]
        private static extern bool FreeLibrary(IntPtr hModule);

        // ── DLL 로드 초기화 ───────────────────────────────────
        static NativeInterop()
        {
            try
            {
                _dll = LoadLibrary(DllPath);
                if (_dll == IntPtr.Zero) return;

                IntPtr pFind = GetProcAddress(_dll, "FindFFmpegPath");
                if (pFind != IntPtr.Zero) _findFFmpeg = Marshal.GetDelegateForFunctionPointer<FindFFmpegPathDelegate>(pFind);

                IntPtr pFormat = GetProcAddress(_dll, "FormatBytes");
                if (pFormat != IntPtr.Zero) _formatBytes = Marshal.GetDelegateForFunctionPointer<FormatBytesDelegate>(pFormat);

                IntPtr pKill = GetProcAddress(_dll, "KillProcessById");
                if (pKill != IntPtr.Zero) _killProcess = Marshal.GetDelegateForFunctionPointer<KillProcessByIdDelegate>(pKill);
            }
            catch { /* DLL 로드 실패 시 폴백 모드로 동작 */ }
        }

        // ── Public API ────────────────────────────────────────

        /// <summary>ffmpeg.exe 폴더 경로 반환. 없으면 빈 문자열.</summary>
        public static string FindFFmpegPath()
        {
            if (_findFFmpeg == null) return "";
            var buf = new byte[512];
            int ok  = _findFFmpeg(buf, buf.Length);
            return ok == 1 ? Encoding.UTF8.GetString(buf).TrimEnd('\0') : "";
        }

        /// <summary>바이트 수치를 "1.23 GB" 형식의 문자열로 변환.</summary>
        public static string FormatBytes(long bytes)
        {
            if (_formatBytes == null)
                return FallbackFormatBytes(bytes);

            var buf = new byte[32];
            _formatBytes(bytes, buf, buf.Length);
            return Encoding.UTF8.GetString(buf).TrimEnd('\0');
        }

        /// <summary>PID 프로세스 강제 종료.</summary>
        public static bool KillProcessById(int pid)
        {
            if (_killProcess == null) return false;
            return _killProcess(pid) == 1;
        }

        /// <summary>DLL 로드 상태 문자열 반환 (로그 표시용).</summary>
        public static string GetDllStatus()
        {
            if (_dll == IntPtr.Zero) return $"❌ 로드 실패 ({DllPath})";
            return $"✅ 로드 완료 ({Path.GetFileName(DllPath)})";
        }

        // ── C# 폴백: DLL 없을 때 FormatBytes ─────────────────
        private static string FallbackFormatBytes(long bytes)
        {
            const long GB = 1024L * 1024 * 1024;
            const long MB = 1024L * 1024;
            const long KB = 1024L;
            if (bytes >= GB) return $"{bytes / (double)GB:F2} GB";
            if (bytes >= MB) return $"{bytes / (double)MB:F2} MB";
            if (bytes >= KB) return $"{bytes / (double)KB:F2} KB";
            return $"{bytes} B";
        }
    }
}
