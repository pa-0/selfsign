/* Copyright (c) Dmitry "Leo" Kuznetsov 2021 see LICENSE for details */
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <Windows.h>

#pragma comment(lib, "advapi32")
#pragma comment(lib, "shell32")

#define fatal_if(b) do {                                                    \
    bool _b_ = (b);                                                         \
    if (_b_) {                                                              \
        fprintf(stderr, "%s failed GetLastEroor()=%d", #b, GetLastError()); \
        exit(1);                                                            \
    }                                                                       \
} while (0)

static char* cwd;

static void* alloc(size_t n);
static char* str(const char* format, ...);
static bool is_elevated(); // Is process running as Admin or System?
static int  runas_admin(int argc, const char* argv[]);
static void extract(const char* label);

static void create_selfsign_certificate_authority();
static void import_certificate_authority();
static void create_selfsign_certificate();
static void convert_certificate_pvk_into_pfx();
static void sing_code(const char* path);

// self signing executable as explained
// https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows
// all command line utilities from Microsoft
//   signtool.exe, makecert.exe, pvk2pfx.exe
// can be coded directly as a sequence of API calls
// but that is rather laborious.
// All three tools are recently deprecated in favor of
// Microsoft Power Shell commands - that themselves are
// implemented as a sequence of API calls.

int main(int argc, const char* argv[]) {
    int r = 0;
    if (!is_elevated()) {
        r = runas_admin(argc, argv);
    } else {
        cwd = (char*)alloc(MAX_PATH);
        fatal_if(!GetCurrentDirectoryA(MAX_PATH, cwd));
        extract("pvk2pfx_exe");
        extract("makecert_exe");
        extract("signtool_exe");
        create_selfsign_certificate_authority();
        import_certificate_authority();
        create_selfsign_certificate();
        convert_certificate_pvk_into_pfx();
        const char* selfname = argc < 2 ? argv[0] : argv[1];
        char* filename = str("%s", selfname);
        size_t n = strlen(filename);
        // remove ".exe" extension
        if (n > 4 || filename[n - 4] != '.') { filename[n - 4] = 0; }
        char* signed_exe = str("%s.signed.exe", filename);
        fatal_if(!CopyFile(selfname, signed_exe, false));
        sing_code(signed_exe);
        printf("signed: \"%s\"\n", signed_exe);
    }
    return r;
}

#define null NULL

static void* alloc(size_t n) {
    void* a = malloc(n);
    fatal_if(a == null);
    return memset(a, 0, n);
}

static char* str(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsnprintf(null, 0, format, args);
    va_end(args);
    fatal_if(n <= 0);
    // allocated memory will be leaked (intentional simplification)
    char* s = alloc(n + 1);
    va_start(args, format);
    vsnprintf(s, n + 1, format, args);
    va_end(args);
    return s;
}

static void memmap_resourse(const char* label, void* *data, int64_t *bytes) {
    HRSRC res = FindResourceA(null, label, (const char*)RT_RCDATA);
    fatal_if(res == null);
    *bytes = SizeofResource(null, res);
    HGLOBAL g = LoadResource(null, res);
    *data = LockResource(g);
}

static bool exists(const char* path) {
    return access(path, 0) == 0;
}

static void write_fully(const char* filename, const void* data, int64_t bytes) {
    fatal_if(!(0 < bytes && bytes <= 0xFFFFFFFFULL));
    int fd = open(filename, O_CREAT | O_WRONLY | O_BINARY, _S_IREAD | _S_IWRITE);
    fatal_if(fd < 0);
    int k = write(fd, data, (uint32_t)bytes);
    fatal_if(k != bytes);
    fatal_if(close(fd) != 0);
}

static void extract(const char* label) {
    char* exe = str("%s", label);
    size_t n = strlen(exe);
    if (exe[n - 4] == '_') { exe[n - 4] = '.'; }
    const char* path = str("%s\\%s", cwd, exe);
    if (!exists(path)) {
        int64_t bytes = 0;
        void* data = 0;
        memmap_resourse(label, &data, &bytes);
        write_fully(path, data, bytes);
    }
}

static void create_selfsign_certificate_authority() {
    if (!exists("selfsign-ca.pvk") || !exists("selfsign-ca.cer")) {
        unlink("selfsign-ca.pvk");
        unlink("selfsign-ca.cer");
        fatal_if(system("makecert.exe -r -pe -n \"CN=Self Sign CA\" -ss CA"
            " -sr CurrentUser -a sha256 -cy authority -sky signature"
            " -sv selfsign-ca.pvk selfsign-ca.cer") != 0);
    }
}

static void import_certificate_authority() {
    // Import Certificate Authority CA into the Windows certificate store
    system("certutil.exe -user -delstore Root selfsign-ca.cer 2>nul >nul");
    system("certutil.exe -user -delstore trustedpublisher selfsign-ca.cer 2>nul >nul");
    fatal_if(system("certutil.exe -user -addstore Root selfsign-ca.cer 2>nul >nul") != 0);
    fatal_if(system("certutil.exe -user -addstore trustedpublisher selfsign-ca.cer 2>nul >nul") != 0);
}

static void create_selfsign_certificate() {
    if (!exists("selfsign-spc.pvk") || !exists("selfsign-spc.cer")) {
        unlink("selfsign-spc.pvk");
        unlink("selfsign-spc.cer");
        fatal_if(system("makecert.exe -pe -n \"CN=Code selfsign SPC\" -a sha256"
            " -cy end -sky signature"
            " -ic selfsign-ca.cer -iv selfsign-ca.pvk"
            " -sv selfsign-spc.pvk selfsign-spc.cer") != 0);
        unlink("selfsign.pfx");
    }
}

static void convert_certificate_pvk_into_pfx() {
    if (!exists("selfsign.pfx")) {
        fatal_if(system("pvk2pfx -pvk selfsign-spc.pvk -spc selfsign-spc.cer -pfx selfsign.pfx") != 0);
    }
}

static void sing_code(const char* path) {
    const char* command =
    str("signtool sign /fd SHA256 /v /n \"SPC\" /ac selfsign-spc.cer /f selfsign.pfx"
        " /t http://timestamp.digicert.com "
        "/v \"%s\"", path);
    fatal_if(system(command) != 0);
}

static int runas_admin(int argc, const char* argv[]) {
    (void)argc; // unused
    const char* arg = GetCommandLineA() + strlen(argv[0]) + 1;
    while (0 < *arg && *arg <= 0x20) { arg++; }
    static SHELLEXECUTEINFOA sei = {sizeof(SHELLEXECUTEINFOA)};
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI |
                SEE_MASK_NO_CONSOLE;
    sei.hwnd = null;
    sei.lpVerb = "runas";
    sei.lpFile = argv[0];
    sei.lpParameters = arg; // nt authority\system
    sei.lpDirectory = null;
    sei.nShow = SW_HIDE;
    sei.hInstApp = null;
    int r = ShellExecuteEx(&sei) ? 0 : GetLastError();
    fatal_if(r != 0);
    // give child process 2 seconds to complete
    Sleep(2000);
    DWORD exit_code = 0;
    fatal_if(!GetExitCodeProcess(sei.hProcess, &exit_code));
    fatal_if(!CloseHandle(sei.hProcess));
    r = exit_code;
    return r;
}

static bool is_elevated() {
    int elevated = false;
    int r = 0;
    PSID administrators_group = null;
    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY administrators_group_authority = SECURITY_NT_AUTHORITY;
    fatal_if(!AllocateAndInitializeSid(&administrators_group_authority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &administrators_group));
    PSID system_ops = null;
    SID_IDENTIFIER_AUTHORITY system_ops_authority = SECURITY_NT_AUTHORITY;
    fatal_if(!AllocateAndInitializeSid(&system_ops_authority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS,
        0, 0, 0, 0, 0, 0, &system_ops));
    if (administrators_group != null) {
        fatal_if(!CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (system_ops != null && !elevated) {
        fatal_if(!CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (administrators_group != null) { FreeSid(administrators_group); }
    if (system_ops != null) { FreeSid(system_ops); }
    fatal_if(r != 0);
    return elevated;
}

