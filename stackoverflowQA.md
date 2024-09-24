# [Self-Signed Codesigning Certificates (Stack Overflow)](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows)

![](www.stackoverflow.com\stackoverflow.png)
<br/>Asked 16 years ago
<br/>Modified [2 months ago](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows?lastactivity "2024-07-10 08:33:54Z")

#security #code-signing

## Question: How do I create a self-signed certificate for code signing on Windows?

<br/>

How do I create a self-signed certificate for code signing using the Windows SDK?

<br/>

[edited Dec 19, 2023 at 20:36](https://stackoverflow.com/posts/84847/revisions "show all edits to this post")<br/>
[TylerH](https://stackoverflow.com/users/2756409/tylerh)<br/>
asked Sep 17, 2008 at 16:04<br/>
[Roger Lipscombe](https://stackoverflow.com/users/8446/roger-lipscombe)


## Answers 1

#### \*\*Updated Answer:\*\*

If you are using the following Windows versions or later: Windows Server 2012, Windows Server 2012 R2, or Windows 8.1 then [`MakeCert` is now deprecated](https://msdn.microsoft.com/en-us/library/windows/desktop/aa386968\(v=vs.85\).aspx), and Microsoft recommends using [the PowerShell Cmdlet **`New-SelfSignedCertificate`**](https://learn.microsoft.com/en-us/powershell/module/pki/new-selfsignedcertificate).

If you're using an older version such as Windows 7, you'll need to stick with `MakeCert` or another solution. Some people [suggest](https://www.reddit.com/r/PowerShell/comments/3190yr/powershell_40_but_no_newselfsignedcertificate/) the [Public Key Infrastructure PowerShell (PSPKI) Module](https://github.com/PKISolutions/PSPKI).

#### Original Answer:

While you can create a self-signed code-signing certificate (SPC - [Software Publisher Certificate](http://msdn.microsoft.com/en-us/library/8s9b9yaz.aspx)) in one go, I prefer to do the following:

##### Creating a self-signed certificate authority (CA)

```bat
makecert -r -pe -n "CN=My CA" -ss CA -sr CurrentUser ^
         -a sha256 -cy authority -sky signature -sv MyCA.pvk MyCA.cer
```

(`^` = escapes newlines in Batchfiles)

This creates a self-signed (`-r`) certificate, with an exportable private key (`-pe`). It's named "My CA", and should be put in the CA store for the current user. We're using the [SHA-256](http://en.wikipedia.org/wiki/SHA-2) algorithm. The key is meant for signing (`-sky`).

The private key should be stored in the `MyCA.pvk` file, and the certificate in the `MyCA.cer` file.

##### Importing the CA certificate

Because there's no point in having a CA certificate if you don't trust it, you'll need to import it into the Windows certificate store. You _can_ use the Certificates MMC snapin, but from the command line:

```bat
certutil -user -addstore Root MyCA.cer
```

##### Creating a code-signing certificate (SPC)

```bat
makecert -pe -n "CN=My SPC" -a sha256 -cy end ^
         -sky signature ^
         -ic MyCA.cer -iv MyCA.pvk ^
         -sv MySPC.pvk MySPC.cer
```

It is pretty much the same as above, but we're providing an issuer key and certificate (the -ic and -iv switches).

We'll also want to convert the certificate and key into a PFX file:

```bat
pvk2pfx -pvk MySPC.pvk -spc MySPC.cer -pfx MySPC.pfx
```

If you are using a password please use the below

```bat
pvk2pfx -pvk MySPC.pvk -spc MySPC.cer -pfx MySPC.pfx -po fess
```

If you want to protect the PFX file, add the -po switch, otherwise PVK2PFX creates a PFX file with no passphrase.

##### Using the certificate for signing code

```bat
signtool sign /v /f MySPC.pfx ^
              /t http://timestamp.url MyExecutable.exe
```

([See why timestamps may matter](https://stackoverflow.com/a/4417466/57611))

If you import the PFX file into the certificate store (you can use PVKIMPRT or the MMC snapin), you can sign code as follows:

```bat
signtool sign /v /n "Me" /s SPC ^
              /t http://timestamp.url MyExecutable.exe
```

Some possible timestamp URLs for `signtool /t` are:

-   `http://timestamp.verisign.com/scripts/timstamp.dll`

-   `http://timestamp.globalsign.com/scripts/timstamp.dll`

-   `http://timestamp.comodoca.com/authenticode`

-   `http://timestamp.digicert.com`

##### Full Microsoft documentation

-   [signtool](http://msdn.microsoft.com/en-us/library/8s9b9yaz.aspx)
-   [makecert](http://msdn.microsoft.com/en-us/library/bfsktky3.aspx)
-   [pvk2pfx](http://msdn.microsoft.com/en-us/library/windows/hardware/ff550672\(v=vs.85\).aspx)

##### Downloads

For those who are not .NET developers, you will need a copy of the Windows SDK and .NET framework. A current link is available here: [SDK & .NET][5] (which installs `makecert` in `C:\Program Files\Microsoft SDKs\Windows\v7.1`). Your mileage may vary.

`MakeCert` is available from the Visual Studio Command Prompt. Visual Studio 2015 does have it, and it can be launched from the Start Menu in Windows 7 under "Developer Command Prompt for VS 2015" or "VS2015 x64 Native Tools Command Prompt" (probably all of them in the same folder).

[Share](https://stackoverflow.com/a/201277 "Short permalink to this answer")
[edited Dec 16, 2021 at 11:20](https://stackoverflow.com/posts/201277/revisions "show all edits to this post")
[shamp00](https://stackoverflow.com/users/1077279/shamp00)
answered Oct 14, 2008 at 14:03

#### Comments

-   Is there any way to populate the certificate's email address field using this method? Right click exe>properties>digital signatures shows email as "not available" after signing. – [cronoklee](https://stackoverflow.com/users/661643/cronoklee "6,662 reputation") [commented May 17, 2012 at 10:52](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment13784467_201277)
    
-   If you get "too many parameters" errors then you check you didn't edit out one of the hyphens accidentally. Failing that - retype the hyphens - don't copy paste. – [fiat](https://stackoverflow.com/users/1141876/fiat "15,881 reputation") [commented Aug 14, 2012 at 0:08](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment15912107_201277)
    
-    @cronoklee To populate the email field of the certificate, simply add `E=your@email`. Eg: `makecert -pe -n "CN=My SPC,E=email@domain" ........`  – [Rob W](https://stackoverflow.com/users/938089/rob-w "347,430 reputation") [Commented Feb 2, 2013 at 11:41](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment20490421_201277)
    
-    Don't you need the Extended Use key flag `-eku 1.3.6.1.5.5.7.3.3` so the cert can be used for code signing (I know powershell fails to sign scripts if it is missing it)  – [Scott Chamberlain](https://stackoverflow.com/users/80274/scott-chamberlain "127,178 reputation") [Commented Feb 20, 2013 at 23:02](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment21055459_201277)
    
-   Yes. `Set-AuthenticodeSignature` is much more picky about which certificates it will accept. It also _requires_ separate CA and SPC certificates.  – [Roger Lipscombe](https://stackoverflow.com/users/8446/roger-lipscombe "91,217 reputation") [Commented Feb 21, 2013 at 12:24](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment21073161_201277) 
    
-   @Scott and Roger: Either of you know why Set-AuthenticodeSignature won't work with a self-signed (or issuer signed for that matter) certificate with 'All applications' and ' All purposes' in Certificate MMC?  – [Mike Cheel](https://stackoverflow.com/users/426422/mike-cheel "13,036 reputation") [Commented Jul 13, 2013 at 5:24](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment25663638_201277)
    
-   @MikeCheel As to "why" only Microsoft can say. You may be better off asking on their forums for an answer.  – [Scott Chamberlain](https://stackoverflow.com/users/80274/scott-chamberlain "127,178 reputation") [Commented Jul 13, 2013 at 5:26](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment25663652_201277)
    
-   It's not necessary to get the Windows SDK to get these files. They are quite small and self contained so they can be downloaded individually.  – [Niels Brinch](https://stackoverflow.com/users/392362/niels-brinch "3,422 reputation") [Commented Aug 1, 2013 at 22:25](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment26330712_201277)
    
-   I noticed you commented that your example is good for "test/internal purposes" is this method acceptable for distribution? (Allowing enough people to click "run anyway" so windows can learn to trust your certificate?)  – [Adam Phelps](https://stackoverflow.com/users/873217/adam-phelps "479 reputation") [Commented Mar 10, 2014 at 7:58](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment33872631_201277)
    
-    @AdamPhelps, Windows won't "learn" to trust your certificate. Your users need to install the CA certificate in the Root store. This is, generally speaking, a bad idea (because root CA certificates can be used for nefarious purposes). It _can_ make sense in an enterprise scenario, though.  – [Roger Lipscombe](https://stackoverflow.com/users/8446/roger-lipscombe "91,217 reputation") [Commented Mar 10, 2014 at 8:11](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment33872974_201277)
    
-   @RogerLipscombe Thanks for the help. I read [this bit](http://blogs.msdn.com/b/ie/archive/2011/03/22/smartscreen-174-application-reputation-building-reputation.aspx) from Microsoft about Smartscreen and code signing. I understood it to apply to self-signed code as well. So to clarify, this is only for Authenticode CAs?  – [Adam Phelps](https://stackoverflow.com/users/873217/adam-phelps "479 reputation") [Commented Mar 10, 2014 at 9:30](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment33875271_201277)
    
-   I think there's a typo, and that you may have switched the /n and /s flags in the last example. The first line should probably read: `signtool sign /v /s Me /n SPC /d http://www.me.me ^`, right?  – [Oskar Lindberg](https://stackoverflow.com/users/2319351/oskar-lindberg "2,285 reputation") [Commented Oct 29, 2014 at 17:35](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment41881411_201277) 
    
-   @OskarLindberg I guess that it's correct and not switched. The /n is used for the subject name and the /s is used for the store name according to the documentation  – [Bernardo Ramos](https://stackoverflow.com/users/4626775/bernardo-ramos "4,509 reputation") [Commented Jul 24, 2016 at 6:17](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment64489886_201277)
    
-   It's in the answer: "`MakeCert` is available from the Visual Studio Command Prompt"  – [Roger Lipscombe](https://stackoverflow.com/users/8446/roger-lipscombe "91,217 reputation") [Commented Nov 28, 2016 at 18:54](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment68919532_201277)
    
-   I tried this method to avoid my .exe to be recognized as "This file might be dangerous" by antivirus software, but it didn't help. Should I use a paid CA provider instead? Any experience about this?  – [Basj](https://stackoverflow.com/users/1422096/basj "45,454 reputation") [Commented Jul 29, 2017 at 14:02](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment77740748_201277)
    
-   Hi @RogerLipscombe, I used this process to sign an XLL. All seems to be fine for the certificates, but Excel still won't accept the Add-In. I wrote a description here [stackoverflow.com/questions/47547387/…](https://stackoverflow.com/questions/47547387/self-signed-code-signing-certificate-for-excel-add-in "self signed code signing certificate for excel add in"). Would you mind taking a look and seeing if you could spot something wrong? I'd be very grateful!  – [MarkNS](https://stackoverflow.com/users/57215/markns "3,991 reputation") [Commented Nov 29, 2017 at 7:41](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment82051393_201277)
    
-   Looks like the [Windows 8 version](https://learn.microsoft.com/en-us/powershell/module/pkiclient/new-selfsignedcertificate?view=winserver2012-ps) of `New-SelfSignedCertificate` does not allow a `-Type` parameter. I cannot generate a code signing certificate with this :/  – [Cardinal System](https://stackoverflow.com/users/5645656/cardinal-system "3,162 reputation") [Commented Jan 25, 2019 at 0:23](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment95529473_201277)
    
-   I tried on Windows10. All commands still work except for the signing using the pfx. I got `SignTool Error: No certificates were found that met all the given criteria.`  – [jaques-sam](https://stackoverflow.com/users/2522849/jaques-sam "2,767 reputation") [Commented Feb 27, 2020 at 9:05](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment106902164_201277)
    
-   I'm having big problems with the new and recommended `New-SelfSignedCertificate` with which I'm trying to clone an existing expired certificate using `-CloneCert`. It is documented to use the same key algorithm as the original certificate, instead it resets the algorithm to rsa1 from rsa256.  – [GSerg](https://stackoverflow.com/users/11683/gserg "77,922 reputation") [Commented Nov 30, 2020 at 16:16](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment115049978_201277)


## Answer 2

As stated in the answer, in order to use a non deprecated way to sign your own script, one should use [`New-SelfSignedCertificate`](https://learn.microsoft.com/en-us/powershell/module/pki/new-selfsignedcertificate?view=windowsserver2022-ps).

1.  Generate the key:
    
    ```powershell
    New-SelfSignedCertificate -DnsName email@yourdomain.com -Type CodeSigning -CertStoreLocation cert:\CurrentUser\My
    ```

2.  Export the certificate without the private key:
    
    ```powershell
    Export-Certificate -Cert (Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert)[0] -FilePath code_signing.crt
    ```
    
    The [0] will make this work for cases when you have more than one certificate... Obviously make the index match the certificate you want to use... or use a way to filtrate (by thumprint or issuer).

3.  Import it as Trusted Publisher
    
    ```powershell
    Import-Certificate -FilePath .\code_signing.crt -Cert Cert:\CurrentUser\TrustedPublisher
    ```

4.  Import it as a Root certificate authority.
    
    ```powershell
    Import-Certificate -FilePath .\code_signing.crt -Cert Cert:\CurrentUser\Root
    ```

5.  Sign the script (assuming here it's named script.ps1, fix the path accordingly).
    
    ```powershell
    Set-AuthenticodeSignature .\script.ps1 -Certificate (Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert)
    ```

Obviously once you have setup the key, you can simply sign any other scripts with it.  

You can get more detailed information and some troubleshooting help in [this article](https://sid-500.com/2017/10/26/how-to-digitally-sign-powershell-scripts/).

[Share](https://stackoverflow.com/a/51443366 "Short permalink to this answer")
[edited Aug 5, 2022 at 4:27](https://stackoverflow.com/posts/51443366/revisions "show all edits to this post")
[hyperblah5](https://stackoverflow.com/users/11428187/hyperblah5)
answered Jul 20, 2018 at 13:14
[chaami](https://stackoverflow.com/users/2647222/chaami)


#### Comments

-   Brilliant except for `script.ps1` which comes out of nowhere even though it must be self-evident to everyone but me. Darn thing. I get the error `File script.ps1 was not found` and that's that. – user1908746 [commented Dec 4, 2019 at 4:49](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment104561364_51443366)
    
-   @Lara thanks for the feedback I have added some contextual info to make it easier even when trying on low caffeine ;-) – [chaami](https://stackoverflow.com/users/2647222/chaami "1,474 reputation") [commented Dec 5, 2019 at 13:27](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment104610569_51443366)
    
-   @chammi, the quick reply is much appreciated. Though, for some reason, all I see now is empty gray rectangles. All the code is gone from the 'code' blocks. Just the gray background is shown. – user1908746 [commented Dec 6, 2019 at 19:53](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment104654510_51443366)
    
-   @Lara thanks for signaling, I hadn't paid close attention while editing, It seems StackOverflow is now more picky about the syntax of the blocks and now requires a new line before the beginning of the code. – [chaami](https://stackoverflow.com/users/2647222/chaami "1,474 reputation") [commented Dec 7, 2019 at 23:33](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment104675446_51443366) 
    
-   I was able to use this answer to sign a .exe that was compiled from python code. and windows SIGNTOOL shows verified, but when I share other users still get a windows defender pop up, any ideas? – [Tyger Guzman](https://stackoverflow.com/users/3835465/tyger-guzman "758 reputation") [commented May 12, 2020 at 17:31](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment109238867_51443366)
    
-   @Tyger, have you tried to give the other users the .crt and asked them to add it as trusted publisher as explained in step 3 ? – [chaami](https://stackoverflow.com/users/2647222/chaami "1,474 reputation") [commented May 25, 2020 at 13:12](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment109661748_51443366)
    
-    last command should also have a proper index :
     ```powershell
     Set-AuthenticodeSignature ... -Certificate (Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert)[1]
     ```
     – [Spongebob Comrade](https://stackoverflow.com/users/970420/spongebob-comrade "1,548 reputation") [commented Apr 30, 2022 at 0:56](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment127334142_51443366)
    
-   Well, that was a complete waste of time - "Your connection is not private" – [Paul McCarthy](https://stackoverflow.com/users/2158599/paul-mccarthy "890 reputation") [commented Feb 3 at 17:19](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment137391918_51443366)
    
-   Anyone struggling with the script.ps1... That is ONLY if you are signing a PowerShell script. Omit that command, as your certificate is already created in the current folder and ready to be used for your application. – [Jeremy Hodge](https://stackoverflow.com/users/1390170/jeremy-hodge "632 reputation") [commented Feb 20 at 19:26](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment137561709_51443366)
    

## Answer 3

It's fairly easy using the [`New-SelfSignedCertificate`](https://learn.microsoft.com/en-us/powershell/module/pki/new-selfsignedcertificate) command in PowerShell. Open PowerShell and run these 3 commands.

> 1. ##### Create certificate:
>     
>     ```powershell
>     $cert = New-SelfSignedCertificate -DnsName www.yourwebsite.com -Type CodeSigning -CertStoreLocation Cert:\CurrentUser\My
>     ```
>     
> 2. ##### Set the password for it:
>     
>     ```powershell
>     $CertPassword = ConvertTo-SecureString -String "my_passowrd" -Force -AsPlainText
>     ```
>     
> 3. ##### Export it:
>     
>     ```powershell
>     Export-PfxCertificate -Cert "cert:\CurrentUser\My\$($cert.Thumbprint)" -FilePath "d:\selfsigncert.pfx" -Password $CertPassword
>     ```
 

Your certificate `selfsigncert.pfx` will be located @ `D:/`

***

###### Optional step:

You would also require to add certificate password to system environment variables. do so by entering below in cmd:

```bat
setx CSC_KEY_PASSWORD "my_password"
```

[Share](https://stackoverflow.com/a/47144138 "Short permalink to this answer")
[edited Feb 23, 2023 at 15:23](https://stackoverflow.com/posts/47144138/revisions "show all edits to this post")
[double-beep](https://stackoverflow.com/users/10607772/double-beep)
answered Nov 6, 2017 at 19:15
[GorvGoyl](https://stackoverflow.com/users/3073272/gorvgoyl)

#### Comments

-   JerryGoyal do you know how to convert a self signed certificate into a CA Root Trusted Certificate?  – [Mr Heelis](https://stackoverflow.com/users/3865138/mr-heelis "2,486 reputation") [Commented Nov 29, 2017 at 9:59](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment82056327_47144138)
    
-    There was a typo in the last script. I should be `Export-PfxCertificate -Cert "cert:\CurrentUser\My\$($cert.Thumbprint)" -FilePath "d:\selfsigncert.pfx" -Password $CertPassword`  – [Arvind Sedha](https://stackoverflow.com/users/1045321/arvind-sedha "101 reputation") [Commented May 17, 2022 at 7:28](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment127680357_47144138) 
    

## Answer 4


Roger's answer was very helpful.

I had a little trouble using it, though, and kept getting the red "Windows can't verify the publisher of this driver software" error dialog. The key was to install the test root certificate with

```bat
certutil -addstore Root Demo_CA.cer
```

which Roger's answer didn't quite cover.

Here is a batch file that worked for me (with my .inf file, not included). It shows how to do it all from start to finish, with no GUI tools at all (except for a few password prompts).

```bat
REM Demo of signing a printer driver with a self-signed test certificate.
REM Run as administrator (else devcon won't be able to try installing the driver)
REM Use a single 'x' as the password for all certificates for simplicity.

PATH %PATH%;"c:\Program Files\Microsoft SDKs\Windows\v7.1\Bin";"c:\Program Files\Microsoft SDKs\Windows\v7.0\Bin";c:\WinDDK\7600.16385.1\bin\selfsign;c:\WinDDK\7600.16385.1\Tools\devcon\amd64

makecert -r -pe -n "CN=Demo_CA" -ss CA -sr CurrentUser ^
   -a sha256 -cy authority -sky signature ^
   -sv Demo_CA.pvk Demo_CA.cer

makecert -pe -n "CN=Demo_SPC" -a sha256 -cy end ^
   -sky signature ^
   -ic Demo_CA.cer -iv Demo_CA.pvk ^
   -sv Demo_SPC.pvk Demo_SPC.cer

pvk2pfx -pvk Demo_SPC.pvk -spc Demo_SPC.cer ^
   -pfx Demo_SPC.pfx ^
   -po x

inf2cat /drv:driver /os:XP_X86,Vista_X64,Vista_X86,7_X64,7_X86 /v

signtool sign /d "description" /du "www.yoyodyne.com" ^
   /f Demo_SPC.pfx ^
   /p x ^
   /v driver\demoprinter.cat

certutil -addstore Root Demo_CA.cer

rem Needs administrator. If this command works, the driver is properly signed.
devcon install driver\demoprinter.inf LPTENUM\Yoyodyne_IndustriesDemoPrinter_F84F

rem Now uninstall the test driver and certificate.
devcon remove driver\demoprinter.inf LPTENUM\Yoyodyne_IndustriesDemoPrinter_F84F

certutil -delstore Root Demo_CA
```

[Share](https://stackoverflow.com/a/16027204 "Short permalink to this answer")
[edited May 18, 2016 at 8:39](https://stackoverflow.com/posts/16027204/revisions "show all edits to this post")
[Peter Mortensen](https://stackoverflow.com/users/63550/peter-mortensen)
answered Apr 16, 2013 at 1:16
[Dan Kegel](https://stackoverflow.com/users/1539692/dan-kegel)

#### Comments

-    If you want to use this for signing drivers, you need to import the CA certificate into the machine store. My example imports it into the user store, which is fine for most software, for test/internal purposes.  – [Roger Lipscombe](https://stackoverflow.com/users/8446/roger-lipscombe "91,217 reputation") [Commented Oct 5, 2013 at 8:25](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment28403071_16027204)
    

## Answer 5

As of PowerShell 4.0 (Windows 8.1/[Server 2012](https://en.wikipedia.org/wiki/Windows_Server_2012) R2) it is possible to make a certificate in Windows without [makecert.exe](https://msdn.microsoft.com/en-us/library/windows/desktop/aa386968%28v=vs.85%29.aspx).

The commands you need are [New-SelfSignedCertificate](https://technet.microsoft.com/library/hh848633) and [Export-PfxCertificate](https://technet.microsoft.com/en-us/library/hh848635.aspx).

Instructions are in _[Creating Self Signed Certificates with PowerShell](https://www.itprotoday.com/blog/creating-self-signed-certificates-powershell)_.

[Share](https://stackoverflow.com/a/35684904 "Short permalink to this answer")
[edited Feb 1, 2021 at 16:38](https://stackoverflow.com/posts/35684904/revisions "show all edits to this post")
[zett42](https://stackoverflow.com/users/7571258/zett42)
answered Feb 28, 2016 at 16:12
[Yishai](https://stackoverflow.com/users/77779/yishai)

-    It's worth mentioning that, even if you install the WMF update to get PowerShell 4.0 on Windows 7, you won't have access to this command. It seems to be Win8 or Server 2012 or later.  – [Daniel Yankowsky](https://stackoverflow.com/users/120278/daniel-yankowsky "6,986 reputation") [Commented Apr 19, 2016 at 21:48](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment61043970_35684904)
    

## Answer 6

For device drivers, you can generate one in Visual Studio 2019, in the project properties. In the Driver Signing section, the Test Certificate field has a drop-down. Generating a test certificate is one of the options. The certificate will be in a file with the 'cer' extension typically in the same output directory as your executable or driver.

Open your project's properties:

[![screenshot](https://i.sstatic.net/vuCgv6o7.png)](https://i.sstatic.net/vuCgv6o7.png)

Then open the 'Driver Signing' section and click on General on the left side. Click on the drop down to the right of 'Test Certificate' and select '<Create Test Certificate...>'

[![screenshot](https://i.sstatic.net/4QgJfdLj.png)](https://i.sstatic.net/4QgJfdLj.png)

[Share](https://stackoverflow.com/a/69534070 "Short permalink to this answer")
[edited May 16 at 23:18](https://stackoverflow.com/posts/69534070/revisions "show all edits to this post")
answered Oct 12, 2021 at 1:23
[trindflo](https://stackoverflow.com/users/3285233/trindflo)

#### Comments

-   Do you have further details like screenshots? Nothing like this appears to exist in Visual Studio 2022.  – [Graham Leggett](https://stackoverflow.com/users/4598583/graham-leggett "1,107 reputation") [Commented May 13 at 12:41](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment138343398_69534070)
    
-   It looks like for Visual Studio 2022 what you want to do is go to 'Publish' and there is an ability to create a test certificate in ClickOnce. I am not on that version of Visual Studio yet, so treat this as hearsay.  – [trindflo](https://stackoverflow.com/users/3285233/trindflo "339 reputation") [Commented May 16 at 23:13](https://stackoverflow.com/questions/84847/how-do-i-create-a-self-signed-certificate-for-code-signing-on-windows#comment138379999_69534070)
    

## Answer 7

This post will only answer the "how to sign an EXE file if you have the certificate" part:

[Signing a Windows EXE file](https://stackoverflow.com/questions/252226/signing-a-windows-exe-file)

To sign the exe file, I used MS "signtool.exe". For this you will need to download the bloated MS Windows SDK which has a whooping 1GB. FORTUNATELY, you don't have to install it. Just open the ISO and extract "Windows SDK Signing Tools-x86_en-us.msi". It has a merely 400 KB.

Then I built this tiny script file:

```bat
prompt $
echo off
cls

copy "my.exe" "my.bak.exe"

"c:\Program Files (x86)\Windows Kits\10\bin\10.0.22000.0\x64\signtool.exe" sign /fd SHA256 /f MyCertificate.pfx /p MyPassword My.exe

pause 
```

__

[Share](https://stackoverflow.com/a/71770052 "Short permalink to this answer")
[edited Jul 10 at 8:33](https://stackoverflow.com/posts/71770052/revisions "show all edits to this post")
answered Apr 6, 2022 at 16:01
[IceCold](https://stackoverflow.com/users/46207/icecold)
