<#
eolconverter.ps1
(C) 2026 Mauro H. Leggieri
License: MIT
Source: https://gist.github.com/mxmauro/0d18e78b2e4b8d095e30a5e4c523b69e

Usage:
    powershell -ExecutionPolicy Bypass -File .\eolconverter.ps1 <crlf|lf|cr> <pattern>

Description:
    Recursively scans files under the current directory, matches their normalized
    relative paths against the supplied pattern, converts line endings, and only
    saves files whose content actually changes.

Pattern syntax:
    /           Path separator
    \\          Alternate path separator
    \x          Escapes the next character x
    *           Matches zero or more non-separator characters
    **          Matches zero or more directory segments
    (...)       Grouping
    a|b         Alternation
    [abc]       Character class
    [!abc]      Negated character class
    ?           Makes the previous character, class, wildcard, or group optional

Examples:
    powershell -ExecutionPolicy Bypass -File .\eolconverter.ps1 lf "**/*.txt"
    powershell -ExecutionPolicy Bypass -File .\eolconverter.ps1 crlf "src/(foo|bar)/**/*.ps1"
    powershell -ExecutionPolicy Bypass -File .\eolconverter.ps1 cr "tmp2\\(utf8|ansi)-nobom[.]txt"
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('crlf', 'lf', 'cr')]
    [string]$EolType,

    [Parameter(Mandatory = $true, Position = 1)]
    [string]$Pattern
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Get-TargetEol {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    switch ($Name.ToLowerInvariant()) {
        'crlf' { return "`r`n" }
        'lf' { return "`n" }
        'cr' { return "`r" }
        default { throw "Unsupported EOL type: $Name" }
    }
}

function Convert-GlobToRegex {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Glob
    )

    $builder = New-Object System.Text.StringBuilder
    [void]$builder.Append('^')
    $groupDepth = 0
    $canQuantifyPrevious = $false

    $i = 0
    while ($i -lt $Glob.Length) {
        $char = $Glob[$i]

        if ($char -eq '\') {
            if (($i + 1) -ge $Glob.Length) {
                throw "Pattern ends with an escape character."
            }

            $nextChar = $Glob[$i + 1]
            if ($nextChar -eq '\') {
                [void]$builder.Append('/')
            }
            else {
                [void]$builder.Append([System.Text.RegularExpressions.Regex]::Escape([string]$nextChar))
            }

            $canQuantifyPrevious = $true
            $i += 2
            continue
        }

        if ($char -eq '*') {
            $hasDoubleStar = ($i + 1 -lt $Glob.Length) -and ($Glob[$i + 1] -eq '*')
            if ($hasDoubleStar) {
                $followedBySeparator = $false
                if (($i + 2) -lt $Glob.Length) {
                    if ($Glob[$i + 2] -eq '/') {
                        $followedBySeparator = $true
                    }
                    elseif ($Glob[$i + 2] -eq '\' -and ($i + 3) -lt $Glob.Length -and $Glob[$i + 3] -eq '\') {
                        $followedBySeparator = $true
                    }
                }

                if ($followedBySeparator) {
                    [void]$builder.Append('(?:[^/]+/)*')
                    if ($Glob[$i + 2] -eq '/') {
                        $i += 3
                    }
                    else {
                        $i += 4
                    }
                    $canQuantifyPrevious = $true
                    continue
                }

                [void]$builder.Append('.*')
                $i += 2
                $canQuantifyPrevious = $true
                continue
            }

            [void]$builder.Append('[^/]*')
            $i += 1
            $canQuantifyPrevious = $true
            continue
        }

        if ($char -eq '/') {
            [void]$builder.Append('/')
            $i += 1
            $canQuantifyPrevious = $false
            continue
        }

        if ($char -eq '(') {
            [void]$builder.Append('(?:')
            $groupDepth += 1
            $i += 1
            $canQuantifyPrevious = $false
            continue
        }

        if ($char -eq ')') {
            if ($groupDepth -le 0) {
                throw "Pattern has an unmatched closing parenthesis."
            }

            [void]$builder.Append(')')
            $groupDepth -= 1
            $i += 1
            $canQuantifyPrevious = $true
            continue
        }

        if ($char -eq '|') {
            [void]$builder.Append('|')
            $i += 1
            $canQuantifyPrevious = $false
            continue
        }

        if ($char -eq '[') {
            $classBuilder = New-Object System.Text.StringBuilder
            [void]$classBuilder.Append('[')
            $i += 1

            if ($i -ge $Glob.Length) {
                throw "Pattern has an unterminated character class."
            }

            if ($Glob[$i] -eq '!' -or $Glob[$i] -eq '^') {
                [void]$classBuilder.Append('^')
                $i += 1
            }

            $classHasContent = $false
            while ($i -lt $Glob.Length) {
                $classChar = $Glob[$i]

                if ($classChar -eq '\') {
                    if (($i + 1) -ge $Glob.Length) {
                        throw "Pattern ends with an escape character inside a character class."
                    }

                    $escapedClassChar = $Glob[$i + 1]
                    if ($escapedClassChar -in @('\', ']', '[', '-', '^')) {
                        [void]$classBuilder.Append('\')
                    }

                    [void]$classBuilder.Append($escapedClassChar)
                    $classHasContent = $true
                    $i += 2
                    continue
                }

                if ($classChar -eq ']') {
                    if (-not $classHasContent) {
                        throw "Pattern has an empty character class."
                    }

                    [void]$classBuilder.Append(']')
                    $i += 1
                    break
                }

                if ($classChar -eq '/') {
                    [void]$classBuilder.Append('/')
                }
                else {
                    if ($classChar -in @('[', ']')) {
                        [void]$classBuilder.Append('\')
                    }

                    [void]$classBuilder.Append($classChar)
                }

                $classHasContent = $true
                $i += 1
            }

            if ($classBuilder[$classBuilder.Length - 1] -ne ']') {
                throw "Pattern has an unterminated character class."
            }

            [void]$builder.Append($classBuilder.ToString())
            $canQuantifyPrevious = $true
            continue
        }

        if ($char -eq '?') {
            if (-not $canQuantifyPrevious) {
                throw "Pattern has '?' without a valid preceding character, set, wildcard, or group."
            }

            [void]$builder.Append('?')
            $i += 1
            $canQuantifyPrevious = $true
            continue
        }

        [void]$builder.Append([System.Text.RegularExpressions.Regex]::Escape([string]$char))
        $i += 1
        $canQuantifyPrevious = $true
    }

    if ($groupDepth -ne 0) {
        throw "Pattern has an unterminated group."
    }

    [void]$builder.Append('$')
    return $builder.ToString()
}

function Get-RelativeNormalizedPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RootPath,

        [Parameter(Mandatory = $true)]
        [string]$FilePath
    )

    $rootFullPath = [System.IO.Path]::GetFullPath($RootPath)
    $fileFullPath = [System.IO.Path]::GetFullPath($FilePath)

    if (-not $rootFullPath.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $rootFullPath += [System.IO.Path]::DirectorySeparatorChar
    }

    if ($fileFullPath.StartsWith($rootFullPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $fileFullPath.Substring($rootFullPath.Length).Replace('\', '/')
    }

    return $fileFullPath.Replace('\', '/')
}

function Get-FileTextInfo {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath
    )

    $bytes = [System.IO.File]::ReadAllBytes($FilePath)

    if ($bytes.Length -eq 0) {
        return @{
            Bytes        = $bytes
            Encoding     = [System.Text.Encoding]::Default
            HasPreamble  = $false
            Text         = ''
            IsBinaryLike = $false
            UseByteLevel = $true
        }
    }

    $encoding = $null
    $offset = 0

    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $encoding = New-Object System.Text.UTF8Encoding($true)
        $offset = 3
    }
    elseif ($bytes.Length -ge 4 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE -and $bytes[2] -eq 0x00 -and $bytes[3] -eq 0x00) {
        $encoding = [System.Text.Encoding]::UTF32
        $offset = 4
    }
    elseif ($bytes.Length -ge 4 -and $bytes[0] -eq 0x00 -and $bytes[1] -eq 0x00 -and $bytes[2] -eq 0xFE -and $bytes[3] -eq 0xFF) {
        $encoding = New-Object System.Text.UTF32Encoding($true, $true)
        $offset = 4
    }
    elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
        $encoding = [System.Text.Encoding]::Unicode
        $offset = 2
    }
    elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF) {
        $encoding = [System.Text.Encoding]::BigEndianUnicode
        $offset = 2
    }
    else {
        $encoding = [System.Text.Encoding]::Default
        $offset = 0
    }

    $isBinaryLike = $false
    if ($offset -eq 0 -and $encoding -eq [System.Text.Encoding]::Default) {
        foreach ($byte in $bytes) {
            if ($byte -eq 0) {
                $isBinaryLike = $true
                break
            }
        }
    }

    $textLength = $bytes.Length - $offset
    $text = $null
    if ($offset -gt 0) {
        $text = $encoding.GetString($bytes, $offset, $textLength)
    }

    return @{
        Bytes        = $bytes
        Encoding     = $encoding
        HasPreamble  = ($offset -gt 0)
        Text         = $text
        IsBinaryLike = $isBinaryLike
        UseByteLevel = ($offset -eq 0)
    }
}

function Convert-LineEndings {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$TargetEol
    )

    $normalized = $Text.Replace("`r`n", "`n").Replace("`r", "`n")
    if ($TargetEol -eq "`n") {
        return $normalized
    }

    return $normalized.Replace("`n", $TargetEol)
}

function Convert-LineEndingsBytes {
    param(
        [Parameter(Mandatory = $true)]
        [byte[]]$Bytes,

        [Parameter(Mandatory = $true)]
        [string]$TargetEol
    )

    $targetBytes = [System.Text.Encoding]::ASCII.GetBytes($TargetEol)
    $result = New-Object System.Collections.Generic.List[byte]
    $changed = $false
    $targetIsCr = ($targetBytes.Length -eq 1 -and $targetBytes[0] -eq 13)
    $targetIsLf = ($targetBytes.Length -eq 1 -and $targetBytes[0] -eq 10)
    $targetIsCrlf = ($targetBytes.Length -eq 2 -and $targetBytes[0] -eq 13 -and $targetBytes[1] -eq 10)

    $i = 0
    while ($i -lt $Bytes.Length) {
        $current = $Bytes[$i]

        if ($current -eq 13) {
            $sourceIsCrlf = $false
            if (($i + 1) -lt $Bytes.Length -and $Bytes[$i + 1] -eq 10) {
                $sourceIsCrlf = $true
                $i += 2
            }
            else {
                $i += 1
            }

            $result.AddRange($targetBytes)
            if (($sourceIsCrlf -and -not $targetIsCrlf) -or ((-not $sourceIsCrlf) -and -not $targetIsCr)) {
                $changed = $true
            }
        }
        elseif ($current -eq 10) {
            $i += 1
            $result.AddRange($targetBytes)
            if (-not $targetIsLf) {
                $changed = $true
            }
        }
        else {
            $result.Add($current)
            $i += 1
        }
    }

    return @{
        Bytes   = $result.ToArray()
        Changed = $changed
    }
}

function Write-FileTextInfo {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [System.Text.Encoding]$Encoding,

        [Parameter(Mandatory = $true)]
        [bool]$HasPreamble
    )

    $contentBytes = $Encoding.GetBytes($Text)
    if ($HasPreamble) {
        $preamble = $Encoding.GetPreamble()
        if ($preamble.Length -gt 0) {
            $buffer = New-Object byte[] ($preamble.Length + $contentBytes.Length)
            [System.Buffer]::BlockCopy($preamble, 0, $buffer, 0, $preamble.Length)
            [System.Buffer]::BlockCopy($contentBytes, 0, $buffer, $preamble.Length, $contentBytes.Length)
            [System.IO.File]::WriteAllBytes($FilePath, $buffer)
            return
        }
    }

    [System.IO.File]::WriteAllBytes($FilePath, $contentBytes)
}

$targetEol = Get-TargetEol -Name $EolType
$pathRegex = New-Object System.Text.RegularExpressions.Regex((Convert-GlobToRegex -Glob $Pattern), [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
$rootPath = (Get-Location).Path

$matchedCount = 0
$changedCount = 0
$skippedCount = 0

Get-ChildItem -Path $rootPath -File -Recurse | ForEach-Object {
    $relativePath = Get-RelativeNormalizedPath -RootPath $rootPath -FilePath $_.FullName

    if (-not $pathRegex.IsMatch($relativePath)) {
        return
    }

    $matchedCount += 1
    $fileInfo = Get-FileTextInfo -FilePath $_.FullName

    if ($fileInfo.IsBinaryLike) {
        $skippedCount += 1
        Write-Output ("Skipped binary-like file: {0}" -f $relativePath)
        return
    }

    if ($fileInfo.UseByteLevel) {
        $byteConversion = Convert-LineEndingsBytes -Bytes $fileInfo.Bytes -TargetEol $targetEol
        if (-not $byteConversion.Changed) {
            Write-Output ("Unchanged: {0}" -f $relativePath)
            return
        }

        [System.IO.File]::WriteAllBytes($_.FullName, $byteConversion.Bytes)
        $changedCount += 1
        Write-Output ("Converted: {0}" -f $relativePath)
        return
    }

    $updatedText = Convert-LineEndings -Text $fileInfo.Text -TargetEol $targetEol
    if ($updatedText -ceq $fileInfo.Text) {
        Write-Output ("Unchanged: {0}" -f $relativePath)
        return
    }

    Write-FileTextInfo -FilePath $_.FullName -Text $updatedText -Encoding $fileInfo.Encoding -HasPreamble $fileInfo.HasPreamble
    $changedCount += 1
    Write-Output ("Converted: {0}" -f $relativePath)
}

if ($matchedCount -eq 0) {
    Write-Output ("No files matched pattern: {0}" -f $Pattern)
}
else {
    Write-Output ("Matched: {0}, Converted: {1}, Skipped: {2}" -f $matchedCount, $changedCount, $skippedCount)
}
