param(
    [Parameter(Mandatory = $true)]
    [string]$Path
)

$resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
$filePath = $resolved.Path
$bytes = [System.IO.File]::ReadAllBytes($filePath)

if ($bytes.Length -lt 0x100) {
    throw "File is too small to be a PE image: $filePath"
}

$peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
if ($peOffset -lt 0 -or $peOffset + 0x18 -ge $bytes.Length) {
    throw "Invalid PE header offset: $filePath"
}

$signature = [BitConverter]::ToUInt32($bytes, $peOffset)
if ($signature -ne 0x00004550) {
    throw "Invalid PE signature: $filePath"
}

$sectionCount = [BitConverter]::ToUInt16($bytes, $peOffset + 6)
$optionalHeaderSize = [BitConverter]::ToUInt16($bytes, $peOffset + 20)
$optionalHeaderOffset = $peOffset + 24
$magic = [BitConverter]::ToUInt16($bytes, $optionalHeaderOffset)

if ($magic -eq 0x10B) {
    $dataDirectoryOffset = $optionalHeaderOffset + 96
} elseif ($magic -eq 0x20B) {
    $dataDirectoryOffset = $optionalHeaderOffset + 112
} else {
    throw ("Unsupported PE optional header magic: 0x{0:X}" -f $magic)
}

$debugDirectoryOffset = $dataDirectoryOffset + (6 * 8)
if ($debugDirectoryOffset + 8 -gt $bytes.Length) {
    throw "Invalid PE debug directory offset: $filePath"
}

$debugRva = [BitConverter]::ToUInt32($bytes, $debugDirectoryOffset)
$debugSize = [BitConverter]::ToUInt32($bytes, $debugDirectoryOffset + 4)

function Convert-RvaToRawOffset {
    param(
        [uint32]$Rva,
        [byte[]]$ImageBytes,
        [int]$SectionsOffset,
        [int]$Sections
    )

    for ($i = 0; $i -lt $Sections; ++$i) {
        $sectionOffset = $SectionsOffset + (40 * $i)
        $virtualSize = [BitConverter]::ToUInt32($ImageBytes, $sectionOffset + 8)
        $virtualAddress = [BitConverter]::ToUInt32($ImageBytes, $sectionOffset + 12)
        $rawSize = [BitConverter]::ToUInt32($ImageBytes, $sectionOffset + 16)
        $rawPointer = [BitConverter]::ToUInt32($ImageBytes, $sectionOffset + 20)
        $span = [Math]::Max($virtualSize, $rawSize)

        if ($span -eq 0) {
            continue
        }

        if ($Rva -ge $virtualAddress -and $Rva -lt ($virtualAddress + $span)) {
            return [int]($rawPointer + ($Rva - $virtualAddress))
        }
    }

    return -1
}

function Clear-ByteRange {
    param(
        [byte[]]$ImageBytes,
        [int]$Offset,
        [uint32]$Size
    )

    if ($Offset -lt 0 -or $Size -eq 0) {
        return
    }

    $end = [int64]$Offset + [int64]$Size
    if ($end -gt $ImageBytes.Length) {
        throw "Refusing to clear byte range outside file: offset=$Offset size=$Size"
    }

    [Array]::Clear($ImageBytes, $Offset, [int]$Size)
}

$sectionsOffset = $optionalHeaderOffset + $optionalHeaderSize
$clearedRanges = 0

if ($debugRva -ne 0 -and $debugSize -ne 0) {
    $debugRawOffset = Convert-RvaToRawOffset $debugRva $bytes $sectionsOffset $sectionCount
    if ($debugRawOffset -lt 0) {
        throw ("Unable to map PE debug RVA 0x{0:X} to a raw file offset" -f $debugRva)
    }

    for ($entryOffset = $debugRawOffset; $entryOffset -lt ($debugRawOffset + [int]$debugSize); $entryOffset += 28) {
        if ($entryOffset + 28 -gt $bytes.Length) {
            throw "Invalid PE debug directory entry range: $filePath"
        }

        $sizeOfData = [BitConverter]::ToUInt32($bytes, $entryOffset + 16)
        $pointerToRawData = [BitConverter]::ToUInt32($bytes, $entryOffset + 24)
        if ($sizeOfData -ne 0 -and $pointerToRawData -ne 0) {
            Clear-ByteRange $bytes ([int]$pointerToRawData) $sizeOfData
            ++$clearedRanges
        }
    }

    Clear-ByteRange $bytes $debugRawOffset $debugSize
    ++$clearedRanges
}

[Array]::Clear($bytes, $debugDirectoryOffset, 8)
[System.IO.File]::WriteAllBytes($filePath, $bytes)

Write-Host ("Stripped PE debug directory from {0}; clearedRanges={1}" -f $filePath, $clearedRanges)
