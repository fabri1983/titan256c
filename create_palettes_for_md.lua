-- How to run this script:
-- If you want to run a single instance:
--   aseprite -b titan_0.png -script-param ID_PROC=0 -script-param SINGLE_INSTANCE=true --script create_palettes_for_md.lua
-- Run with ID_PROC=0 only to know how much permutation instances do we need to run
--   aseprite -b titan_0.png -script-param ID_PROC=0 --script create_palettes_for_md.lua
-- Once we have that number we can execute the .bat script to parallelize the permutations
--   create_palettes_executor.bat titan_0.png <num_perm_instances>
--   open new cmd and run: taskkill /f /fi "imagename eq aseprite.exe"
----------------------------------------------------------------------------------------------------
-- globals
local sprite = app.activeSprite

-- set chunk size
local tileWidthMD = 8 -- Megadrive tile width
local tileHeightMD = 8 -- Megadrive tile height

-- get sprite size
local spriteWidth = sprite.width
local spriteHeight = sprite.height

-- sprite's filename without extension
local sprFilename = sprite.filename:match("(.+)%..+")

-- count occurrences for each color
local occurrencesByColor = {}
-- keep created Palettes
local palettesAllUnique = {}

local colorTransparent = Color(0,0,0,0)

local ID_PROC = tonumber(app.params["ID_PROC"])

local SINGLE_INSTANCE = false
if app.params["SINGLE_INSTANCE"] == "true" then
  SINGLE_INSTANCE = true
end

local TARGET_PALETTES = 2

local MAX_COLORS_PER_PALETTE = 16 -- first color is the transparent one
local MAX_COLORS_PER_PALETTE_FOR_MD_RGB = 16 -- DO NOT MODIFY

-- In each iteration it tries to add colors in one or more palettes in such a way other palettes end up being fully contained 
-- by any other, thus removing them from the total of palettes used for the permutation algorithm.
local APPROXIMATION_ITERS = 0
local MISSING_COLORS = 1 -- up to N missing colors

local PRINT_PALETTES_SIZE = false and (ID_PROC == 0)
local SAVE_HELPER_FILES = false and (ID_PROC == 0)
local SAVE_PALETTE_FILES = false

----------------------------------------------------------------------------------------------------
local function wait(seconds)
  local start = os.time()
  repeat until os.time() > start + seconds
end

----------------------------------------------------------------------------------------------------
local function fileExists (name)
   local f = io.open(name, "r")
   return f ~= nil and io.close(f)
end

----------------------------------------------------------------------------------------------------
function containsString (array, targetString)
  for i = 1, #array do
    if array[i] == targetString then
      return true
    end
  end
  return false
end

----------------------------------------------------------------------------------------------------
-- Swaps two element from an array
local function swap (arr, i, j)
   arr[i], arr[j] = arr[j], arr[i]
end

----------------------------------------------------------------------------------------------------
-- cond == true ? T : F
local function ternary ( cond , T , F )
   if cond then return T else return F end
end

----------------------------------------------------------------------------------------------------
-- Partition function for Quick sort
local function partition (arr, start_idx, end_idx, comparator)
   local pivot = arr[end_idx]
   local i = start_idx - 1

   for j = start_idx, end_idx - 1 do
      if comparator(arr[j], pivot) then
         i = i + 1
         arr[i], arr[j] = arr[j], arr[i]
      end
   end

   arr[i + 1], arr[end_idx] = arr[end_idx], arr[i + 1]
   return i + 1
end

local function quicksort (arr, start_idx, end_idx, comparator)
   if start_idx < end_idx then
      local pivot_idx = partition(arr, start_idx, end_idx, comparator)
      quicksort(arr, start_idx, pivot_idx - 1, comparator)
      quicksort(arr, pivot_idx + 1, end_idx, comparator)
   end
end

-- Quicksort implementation using 1-based indexed arrays
local function _sort (arr, comparator)
   quicksort(arr, 1, #arr, comparator)
end

----------------------------------------------------------------------------------------------------
-- Generates a compound key given a Color c
local function getColorKey (c)
  return string.format("%d,%d,%d,%d", c.red, c.green, c.blue, c.alpha)
end

----------------------------------------------------------------------------------------------------
-- Generates a compound key given an X and Y cordinate
local function getTileKey (x, y)
  return string.format("%04d_%04d", y, x)
end

----------------------------------------------------------------------------------------------------
-- Checks if 2 colors are equal
local function equalsColor (c1, c2)
  return c1.red == c2.red and c1.green == c2.green and c1.blue == c2.blue and c1.alpha == c2.alpha
end

----------------------------------------------------------------------------------------------------
-- Checks if array colors contains color cc
local function containsColor (colors, cc)
  for i=1, #colors do
     local c = colors[i]
     if equalsColor(cc, c) then
       return true
     end
  end
  return false
end

----------------------------------------------------------------------------------------------------
-- Checks if array colors contains color cc
local function containsColorSinceIndex (colors, cc, ind)
  for i=ind, #colors do
     local c = colors[i]
     if equalsColor(cc, c) then
       return true
     end
  end
  return false
end

----------------------------------------------------------------------------------------------------
local function sortColorsBy_RGB_ASC_func (c1, c2)
   if c1.red < c2.red then
      return true
   elseif c1.red == c2.red then
      if c1.green < c2.green then
         return true
      elseif c1.green == c2.green then
         return c1.blue < c2.blue
      end
   end
   return false
end

local function calculateLuminance(c)
   return 0.2126 * c.red + 0.7152 * c.green + 0.0722 * c.blue
end

local function sortColorsBy_LUMINANCE_DESC_func (c1, c2)
   local luminance1 = calculateLuminance(c1)
   local luminance2 = calculateLuminance(c2)
   -- luminance close to 0 is darker, luminance close to 1 is brighter, so we invert the comparison
   return luminance1 < luminance2
end

----------------------------------------------------------------------------------------------------
local function sortPaletteColors (palette)
  local colorCount = #palette
  local colors = {}

  -- Retrieve all colors from the palette
  for i = 0, colorCount - 1 do -- Palette is 0-based internally
    local color = palette:getColor(i)
    table.insert(colors, color)
  end

  -- Sort colors
  _sort(colors, sortColorsBy_LUMINANCE_DESC_func)

  -- Set the sorted colors back to the palette
  for i = 0, colorCount - 1 do -- Palette is 0-based internally
    local color = colors[i + 1] -- Colors table is 1-indexed, so we offset the index by 1
    palette:setColor(i, color)
  end
end

----------------------------------------------------------------------------------------------------
-- Checks if color exists in Palette pal
local function existsColorInPalette (color, pal)
  for i=0, #pal - 1 do -- Palette is 0-based internally
    local c = pal:getColor(i)
    if equalsColor(color, c) then
      return true
    end
  end
  return false
end

-- Checks if all pal's colors are fully contained by other palette except the current one
local function isContainedInOther(pal, palettes, currPalIndex)
  if #palettes == 0 then
    return false
  end
  for j=1, #palettes do
    local otherPal = palettes[j]
    if j ~= currPalIndex and #pal <= #otherPal then
       local fullyContained = true
       -- check if every color in pal is fully contained in palette otherPal
       for i=0, #pal - 1 do -- Palette is 0-based internally
         local c = pal:getColor(i)
         if not existsColorInPalette(c, otherPal) then
           fullyContained = false
           break
         end
       end
       if fullyContained then
          return true
       end
    end
  end
  return false
end

----------------------------------------------------------------------------------------------------
-- Checks if 2 Palettes are equal in size and colors
local function isTheSamePalette (p1, p2)
  if not p1 or not p2 then
    return false
  end
  if #p1 ~= #p2 then
    return false
  end
  for i=0, #p1 - 1 do -- Palette is 0-based internally
    local c = p1:getColor(i)
    if not existsColorInPalette(c, p2) then
      return false
    end
  end
  return true
end

----------------------------------------------------------------------------------------------------
-- Checks if Palette pal exists in palettes
local function alreadyExists (pal, palettes)
  if #palettes == 0 then
    return false
  end
  for i=1, #palettes do
    local otherPal = palettes[i]
    if isTheSamePalette(pal, otherPal) then
       return true
    end
  end
  return false
end

----------------------------------------------------------------------------------------------------
local function getUniqueColorsArray (palettes)
  local unique = {}

  for i=1, #palettes do
    local pal = palettes[i]
    for j=0, #pal - 1 do -- Palette is 0-based internally
      local color = pal:getColor(j)
      local isUnique = true
      -- search over the current unique colors to see if it already exists
      for k, uniqueColor in ipairs(unique) do
        if equalsColor(color, uniqueColor) then
          isUnique = false
          break
        end
      end
      -- if isUnique and non transparent then keep it
      if isUnique and color.alpha ~= 0 then
        table.insert(unique, color)
      end
    end
  end
  -- sort unique colors
  _sort(unique, sortColorsBy_LUMINANCE_DESC_func)
  return unique
end

----------------------------------------------------------------------------------------------------
local function getColorIndex (color, colorsArr)
  for i=1, #colorsArr do
    if equalsColor(color, colorsArr[i]) then
      return i
    end
  end
  return 1
end

----------------------------------------------------------------------------------------------------
-- If colors array has more than MAX_COLORS_PER_PALETTE - 1 colors (transparent color is mandatory for MD) then remove 
-- the less used colors with help of occurrences map.
local function trimColorsIfExceeded (colors, occurrencesMap, colorsTrimmedMap, tileKey)
   if #colors <= (MAX_COLORS_PER_PALETTE - 1) then
      return colors
   end
   -- copy the colors of the array in a new one
   local colorsSorted = {}
   for i=1, #colors do
      colorsSorted[i] = colors[i]
   end
   -- sort the colors by occurrence in DESC order, so the less used colors are moved to the end
   _sort(colorsSorted, function(c1, c2)
      local key1 = getColorKey(c1)
      local key2 = getColorKey(c2)
      return occurrencesMap[key1] > occurrencesMap[key2]
   end)
   -- create new array containing original colors except those trimmed out
   local colorsFinal = {}
   local t = 1
   for i=1, #colors do
      local c = colors[i]
      if not containsColorSinceIndex(colorsSorted, c, MAX_COLORS_PER_PALETTE) then
         colorsFinal[t] = c
		 t = t + 1
      else
         local colorKey = getColorKey(c)
         colorsTrimmedMap[tileKey .. "-" .. colorKey] = c
      end
   end
   return colorsFinal
end

----------------------------------------------------------------------------------------------------
-- remove palettes that are fully contained in other palette
local function removeFullyContainedPalettes (palettes)
   local newPalettes = {}
   local k = 1
   for i=1, #palettes do
      local pal = palettes[i]
      if not isContainedInOther(pal, palettes, i) then
         newPalettes[k] = pal
         k = k + 1
      end
   end
   return newPalettes
end

-- create a map of color keys counting how many times a color is missing in other palette
local function getMissingColorOccurrs (palettes)
   local occurrs = {}

   for i=1, #palettes do
      local pal = palettes[i]
      for j=1, #palettes do
         local otherPal = palettes[j]
         -- check pal is smaller or same size than otherPal
         if i ~= j and #pal <= #otherPal then
            -- check for pal's missing colors into otherPal, and count them into occurrs
            for i=0, #pal - 1 do -- Palette is 0-based internally
               local c = pal:getColor(i)
               if not existsColorInPalette(c, otherPal) then
                  local key = getColorKey(c)
                  local count = occurrs[key]
                  if count == nil then
                     occurrs[key] = 1
                  else
                     occurrs[key] = count + 1
                  end
               end
            end
         end
      end
   end

   return occurrs
end

-- create an array of N color keys sorted by biggest occurrence
local function getKeysWithBiggestValues (map, N)
  local sortedKeys = {}
  
  -- Sorting the keys based on their values in descending order
  for key, value in pairs(map) do
    table.insert(sortedKeys, key)
  end
  table.sort(sortedKeys, function(a, b) return map[a] > map[b] end)
  
  -- Returning the first N keys with the largest values
  local largestKeys = {}
  for i = 1, math.min(#sortedKeys, N) do
    if sortedKeys[i] then
      table.insert(largestKeys, sortedKeys[i])
    else
      break
    end
  end
  
  return largestKeys
end

-- Create an array containing the colors that otherPal needs to fully contains pal.
-- Ie: returns the elements from pal that don't exist in otherPal.
local function getMissingColorsFrom (pal, otherPal)
  local missingColors = {}
  
  for i=0, #pal - 1 do -- Palette is 0-based internally
    local colorExist = false
    for j=0, #otherPal - 1 do -- Palette is 0-based internally
      if equalsColor(pal:getColor(i), otherPal:getColor(j)) then
        colorExist = true
        break
      end
    end
    if not colorExist then
      table.insert(missingColors, pal:getColor(i))
    end
  end
  
  return missingColors
end

local function addColorToPalette (pal, color)
  local newPal = Palette(#pal + 1)
  for i=0, #pal - 1 do -- Palette is 0-based internally
    newPal:setColor(i, pal:getColor(i))
  end
  newPal:setColor(#pal, color)
  newPal:resize(#pal + 1)
  return newPal
end

----------------------------------------------------------------------------------------------------
-- Computes the distance between two colors using the CIELAB method
local function calculateCIELABDistance (color1, color2)
  -- Convert color values to the sRGB color space
  local r1, g1, b1 = color1.red / 255, color1.green / 255, color1.blue / 255
  local r2, g2, b2 = color2.red / 255, color2.green / 255, color2.blue / 255

  -- Convert from sRGB to XYZ color space
  local function convertToXYZ(color)
    if color > 0.04045 then
      color = ((color + 0.055) / 1.055) ^ 2.4
    else
      color = color / 12.92
    end
    return color * 100
  end

  local x1 = convertToXYZ(r1 * 0.4124 + g1 * 0.3576 + b1 * 0.1805)
  local y1 = convertToXYZ(r1 * 0.2126 + g1 * 0.7152 + b1 * 0.0722)
  local z1 = convertToXYZ(r1 * 0.0193 + g1 * 0.1192 + b1 * 0.9505)

  local x2 = convertToXYZ(r2 * 0.4124 + g2 * 0.3576 + b2 * 0.1805)
  local y2 = convertToXYZ(r2 * 0.2126 + g2 * 0.7152 + b2 * 0.0722)
  local z2 = convertToXYZ(r2 * 0.0193 + g2 * 0.1192 + b2 * 0.9505)

  -- Convert from XYZ to CIELAB color space
  local function convertToLAB(value)
    if value > 0.008856 then
      value = value ^ (1/3)
    else
      value = (903.3 * value + 16) / 116
    end
    return value
  end

  local xL1 = convertToLAB(x1 / 95.047)
  local yL1 = convertToLAB(y1 / 100)
  local zL1 = convertToLAB(z1 / 108.883)

  local xL2 = convertToLAB(x2 / 95.047)
  local yL2 = convertToLAB(y2 / 100)
  local zL2 = convertToLAB(z2 / 108.883)

  -- Calculate the differences between the L*, a* and b* components
  local deltaL = yL1 - yL2
  local deltaA = xL1 - xL2
  local deltaB = zL1 - zL2

  -- Calculate the overall color difference using the CIELAB formula
  local distance = math.sqrt(deltaL^2 + deltaA^2 + deltaB^2)
  return distance
end

-- Given a color c and an array of colors find the most similar color to c
local function findSimilarColor (colors, c)
   local bestMatch = nil
   local bestDistance = 999999

   for i=1, #colors do
      local color = colors[i]
      if color.alpha ~= 0 then
         local distance = calculateCIELABDistance(c, color)
         if distance < bestDistance then
            bestMatch = color
            bestDistance = distance
         end
      end
   end

   return bestMatch
end

----------------------------------------------------------------------------------------------------
local function replaceTrimmedColorsByNearest (allImg, sprPalette, x, y, thisTileWidth, thisTileHeight, uniqueColors, finalUniqueColors)
  -- get trimmed colors
  local trimmedColors = {}
  for i=1, #uniqueColors do
     if not containsColor(finalUniqueColors, uniqueColors[i]) then
        table.insert(trimmedColors, uniqueColors[i])
     end
  end

  -- get nearest color for each trimmed one from the finalUniqueColors
  local nearestColors = {}
  for i=1, #trimmedColors do
     table.insert(nearestColors, findSimilarColor(finalUniqueColors, trimmedColors[i]))
  end

  for yy=y, y + thisTileHeight - 1 do
     for xx=x, x + thisTileWidth - 1 do
        local pi = allImg:getPixel(xx, yy) -- this is an index pointing to the Sprite's Palette
        local c = sprPalette:getColor(pi)
        -- skip transparent color
        if not equalsColor(colorTransparent, c) then
           local trimmedColorIndex = 0
		   -- search for index in trimmedColors
           for ti=1, #trimmedColors do
              if c == trimmedColors[ti] then
                trimmedColorIndex = ti
                break
              end
           end
           -- get the nearest color and replace the current pixel by this one
           if trimmedColorIndex > 0 then
              local nearestColor = nearestColors[trimmedColorIndex]
              -- get the Palettes's index where the nearestColor is located
              local nearestColorPaletteIndex = 0
              for ppi=0, #sprPalette - 1 do -- Palette is 0-based internally
                 local colorInPal = sprPalette:getColor(ppi)
                 nearestColorPaletteIndex = ppi
                 break
              end
              allImg:drawPixel(xx, yy, nearestColorPaletteIndex)
           end
        end
     end
  end
end

----------------------------------------------------------------------------------------------------
local function printColor8x8InImage (img, color, atX, atY)
  for y = atY, atY + 8 - 1 do
    for x = atX, atX + 8 - 1 do
      img:drawPixel(x, y, color)
    end
  end
end

-- Create an array containing the size of each palette. Array palettes already sorted
local function createSizePaletteGrouping (palettes)
  local sizesGrouping = {}
  for i=1, #palettes do
    local pal = palettes[i]
    sizesGrouping[i] = #pal
  end
  return sizesGrouping
end

-- Get a list of the elements in a comma separated list as a string
local function getGroupingsAsStringList (array)
  local result = ""
  for i = 1, #array do
    result = result .. tostring(array[i])
    if i < #array then
      result = result .. ", "
    end
  end
  return result
end

-- Get the next group following the group param
local function getNextGroup (sizesGrouping, group)
  -- find the index where the group starts
  local startAt = #sizesGrouping
  for i=1, #sizesGrouping do
    if sizesGrouping[i] == group then
      startAt = i
      break
    end
  end
  for i=startAt, #sizesGrouping do
    if sizesGrouping[i] ~= group then
      return sizesGrouping[i]
    end
  end
  return sizesGrouping[#sizesGrouping]
end

-- Get the index of the first element belonging to group. Array sizesGrouping is already sorted
local function getGroupStartingIndex (sizesGrouping, group)
  for i=1, #sizesGrouping do
    if sizesGrouping[i] == group then
      return i
    end
  end
  return #sizesGrouping
end

-- Get the index of the last element belonging to group. Array sizesGrouping is already sorted
local function getGroupEndingIndex (sizesGrouping, group)
  -- First find the index where the group starts
  local startAt = getGroupStartingIndex(sizesGrouping, group)
  -- Second find the index at which the group ends
  for i=startAt, #sizesGrouping do
    if sizesGrouping[i] ~= group then
      return i - 1
    end
  end
  return #sizesGrouping
end

-- accesses:         [1,2,3,  4,5,  6,7,   8]
-- sizesGrouping:    [6,6,6,  7,7,  9,9,  14]
-- from = 1, to = 3 (start and end for first group)
-- currGroup = 6
-- lastGroup = 14
-- Recursive function to generate permutations of accesses by groups
local function generatePermutationsByGroup (accesses, from, to, sizesGrouping, currGroup, lastGroup, callback)
   -- are we at base case?
   if from == to then
      -- if we are in the last group then we are at the end of a complete permutation
      if currGroup == lastGroup then
         return callback(accesses)
      end
      -- otherwise continue with next group
	  local nextGroup = getNextGroup(sizesGrouping, currGroup)
      local nextFrom = getGroupStartingIndex(sizesGrouping, nextGroup)
      local nextTo = getGroupEndingIndex(sizesGrouping, nextGroup)
      return generatePermutationsByGroup(accesses, nextFrom, nextTo, sizesGrouping, nextGroup, lastGroup, callback)
   end

   for i = from, to do
      local result = generatePermutationsByGroup(accesses, from, to-1, sizesGrouping, currGroup, lastGroup, callback)
      if result then -- at first succeeded permutation starts propagating the result
         return true
      end
      if to % 2 == 0 then
         swap(accesses, i, to)
      else
         swap(accesses, from, to)
      end
   end

   return false
end

-- Recursive function to generate all possible permutations over accesses array
local function generatePermutations (accesses, n, callback)
   -- Base case: if only one element, do callback
   if n == 1 then
      return callback(accesses)
   end
    
   for i = 1, n do
      local result = generatePermutations(accesses, n-1, callback)
      if result then -- at first succeeded permutation starts propagating the result
         return true
      end
      if n % 2 == 0 then
         swap(accesses, i, n)
      else
         swap(accesses, 1, n)
      end
   end

   return false
end

-- Defines a factorial function
local function fact (n)
  if n <= 0 then
    return 1
  else
    return n * fact(n-1)
  end
end

-- Calculates the total number of permutations giving the groupings
local function computeNumberOfPermutationsWithGroupings (sizesGrouping, startAt)
  local currGroup = sizesGrouping[startAt]
  local accum = 1
  local numRowsInGroup = 0
  for i=startAt, #sizesGrouping do
    -- if we are now in a diferent group then calculates the accumulative product
    if (currGroup ~= sizesGrouping[i]) or (i == #sizesGrouping) then
      if i == #sizesGrouping then -- when on last group we need to count for the missing counted row
        numRowsInGroup = numRowsInGroup + 1
      end
      accum = accum * fact(numRowsInGroup)
      currGroup = sizesGrouping[i]
      numRowsInGroup = 0
    end
    numRowsInGroup = numRowsInGroup + 1
  end
  return accum
end

-- ###########################################################################################################
if not sprite then
   if app.isUIAvailable then
      return app.alert("There is no active sprite")
   else
      print("There is no active sprite")
   end
   goto _done -- terminates the script immediately and without warning
end

if sprite.colorMode ~= ColorMode.INDEXED then
   if app.isUIAvailable then
      return app.alert("The sprite is not indexed")
   else
      print("The sprite is not indexed")
   end
   goto _done -- terminates the script immediately and without warning
end

if #sprite.palettes < 1 then
   if app.isUIAvailable then
      return app.alert("The sprite has no palette")
   else
      print("The sprite has no palette")
   end
   goto _done -- terminates the script immediately and without warning
end

-- ###########################################################################################################
if ID_PROC == 0 or ID_PROC == 1 then
  print("##############################################\n" .. sprite.filename .. "\n##############################################")
end

if ID_PROC == 0 then
  print("-- STEP 1: Iterate over all Sprite pixels and computes each color occurrence.")
end

-- 1: iterate over all pixels in sprite and count the occurrences of each one
for y = 0, spriteHeight - 1 do

   local sprPalette = sprite.palettes[1]
   local allImg = sprite.cels[1].image

   for x = 0, spriteWidth - 1 do
      local pi = allImg:getPixel(x, y) -- this is an index pointing to the Sprite's Palette
      local c = sprPalette:getColor(pi)
      -- skip transparent color
      if not equalsColor(colorTransparent, c) then
         local key = getColorKey(c)
         -- count for global color occurrences in occurrencesByColor map
         local count = occurrencesByColor[key]
         if count == nil then
           occurrencesByColor[key] = 1
         else
           occurrencesByColor[key] = count + 1
         end
      end
   end
end

-- Print total occurrences by color
--for k,v in pairs(occurrencesByColor) do
--  print(k, v)
--end

-- Print Palete indexes of first tile
--local tileImgFirst = Image(sprite.cels[1].image, Rectangle(0, 0, tileWidthMD, tileHeightMD))
--for y = 0, tileImgFirst.height - 1 do
--   local rowStr = ""
--   for x = 0, tileImgFirst.width - 1 do
--      local p = tileImgFirst:getPixel(x, y) -- this is an index pointing to the Sprite's Palette
--      rowStr = string.format("%s %3d", rowStr, p)
--   end
--   print(rowStr)
--end

-- ###########################################################################################################
if ID_PROC == 0 then
  print(string.format("-- STEP 2: Iterate over all %dx%d tile and generates one palette (max size %d) per tile. Total tiles in Sprite: %d", 
     tileWidthMD, tileHeightMD, MAX_COLORS_PER_PALETTE, (spriteWidth*spriteHeight)/(tileWidthMD*tileHeightMD)))
end

-- 2: iterate all tiles in the sprite and generates non repeated and trimmed (MAX_COLORS_PER_PALETTE colors max) Palettes

-- keep track of discarded colors so they can be replaced by a similar one in the current tile
local colorsTrimmedMap = {}

local iForPalettesAll = 1
for y = 0, spriteHeight - 1, tileHeightMD do

   local allImg = sprite.cels[1].image
   local sprPalette = sprite.palettes[1]
   local thisTileHeight = ternary(y + tileHeightMD <= spriteHeight, tileHeightMD, spriteHeight - y)

   for x = 0, spriteWidth - 1, tileWidthMD do

      local tileKey = getTileKey(x, y)
      local thisTileWidth = ternary(x + tileWidthMD <= spriteWidth, tileWidthMD, spriteWidth - x)
      local uniqueColors = {}
	  local kForUnique = 1
      local occurrs = {}

      -- add unique colors
      for yy=y, y + thisTileHeight - 1 do
         for xx=x, x + thisTileWidth - 1 do
            local pi = allImg:getPixel(xx, yy) -- this is an index pointing to the Sprite's Palette
            local c = sprPalette:getColor(pi)
            -- skip transparent color
            if not equalsColor(colorTransparent, c) then
               local key = getColorKey(c)
               -- check for this local occurrence
               local count = occurrs[key]
               if count == nil then
                  occurrs[key] = 1
                  uniqueColors[kForUnique] = c
                  kForUnique = kForUnique + 1
               else
                  occurrs[key] = count + 1
               end
            end
         end
      end

      -- sort unique colors
      _sort(uniqueColors, sortColorsBy_LUMINANCE_DESC_func)

      -- If uniqueColors has more than MAX_COLORS_PER_PALETTE - 1 colors then trim the less used colors.
      -- Might use occurrs instead of occurrencesByColor as occurs counts on local context
      local finalUniqueColors = trimColorsIfExceeded(uniqueColors, occurrencesByColor, colorsTrimmedMap, tileKey) 

      -- if any color was trimmed then replaced them in the curent tile by the nearest color that was kept in finalUniqueColors
      if #uniqueColors ~= #finalUniqueColors then
         replaceTrimmedColorsByNearest(allImg, sprPalette, x, y, thisTileWidth, thisTileHeight, uniqueColors, finalUniqueColors)
      end

      -- create palette to current amount of unique colors
	  local nColorsPal = #finalUniqueColors
	  local palette = Palette(nColorsPal)
      -- assign the sorted and unique colors
      for i=1, nColorsPal do
         palette:setColor(i-1, finalUniqueColors[i]) -- Palette is 0-based internally
      end
      palette:resize(nColorsPal)

      -- add the Palette if it doesn't exist already
      if #palette > 0 and not alreadyExists(palette, palettesAllUnique) then
         palettesAllUnique[iForPalettesAll] = palette
		 iForPalettesAll = iForPalettesAll + 1
      end
   end
end

-- ###########################################################################################################
if ID_PROC == 0 then
  print("-- STEP 3: Remove palettes that are fully contained in other palette. Save some helper image files if you want it so.")
end

-- 3: sort palettesAllUnique array by the size of their palettes
local PALETTES_ORDER_WAY = "DESC"
local function sortPalettesAllUnique_func (p1, p2)
   if PALETTES_ORDER_WAY == "DESC" then
      return #p1 > #p2
   elseif PALETTES_ORDER_WAY == "ASC" then
      return #p1 < #p2
   end
end

_sort(palettesAllUnique, sortPalettesAllUnique_func)

-- remove palettes that are fully contained in other palette
palettesAllUnique = removeFullyContainedPalettes(palettesAllUnique)

-- APPROXIMATION ALGORITHM 1
-- -------------------------
-- The goal of this algorithm is to grow in size some palettes to remove other palettes fully contained by the extended palette, 
-- thus diminishing the total number of palettes used in the permutation algorithm.
-- 1) For every palette smaller or same size than other, build a map counting the occurrences for every color that is not contained.
-- 2) Using that occurrences map, traverse the palettes and for each palette not contained into another because of 1 missing 
-- color, add it to the target palette being tested against to (keeping MAX_COLORS_PER_PALETTE restriction) if the missing 
-- color is in the top N colors with bigger occurrences (from the occurrences map mentioned before). 
-- 3) After this remove palettes fully contained in other palette, traverse the palettesAllUnique pals, sort colors again, and 
-- sort the palettesAllUnique again
-- 4) Repeat the entire process from step 1 as many times you want to run.

if APPROXIMATION_ITERS > 0 then

   for iter=1, APPROXIMATION_ITERS do
      local anyPalUpdated = false

      -- 1)
      local missingColorOccurrsMap = getMissingColorOccurrs(palettesAllUnique)
      local missingColorKeysTopN = getKeysWithBiggestValues(missingColorOccurrsMap, 10)

      -- 2)
      local paletteModified = {}
      for i=1, #palettesAllUnique do
         paletteModified[i] = false
      end

      for i=1, #palettesAllUnique do
         -- this is the palette we assume is not contained in other palette
         local pal = palettesAllUnique[i]
         for j=1, #palettesAllUnique do
            local targetPal = palettesAllUnique[j] -- palette to be extended with additional colors
            -- check pal is smaller or same size than targetPal
            if i ~= j and not paletteModified[j] and #pal <= #targetPal then
               local missingColors = getMissingColorsFrom(pal, targetPal)
               -- check if pal has desired amount of missing colors in targetPal
               if #missingColors >= 1 and #missingColors <= MISSING_COLORS then
                  for k=1, #missingColors do
                     local missingColor = missingColors[k]
                     local colorKey = getColorKey(missingColor)
                     -- check if it's one of the top N missing colors
                     if containsString(missingColorKeysTopN, colorKey) then
                        -- check that targetPal size is smaller than (MAX_COLORS_PER_PALETTE - 1) - 1:
                        --  -1: transparent color is added further in the process of buckets generation
                        --  -1: the missing color from pal we want to add into targetPal
                        if #targetPal < ((MAX_COLORS_PER_PALETTE - 1) - 1) then
                           -- add the color into targetPal
                           local newOtherPal = addColorToPalette(targetPal, missingColor)
						   targetPal = newOtherPal
                           -- save the update palette
                           palettesAllUnique[j] = newOtherPal
                           paletteModified[j] = true
						   anyPalUpdated = true
                        end
                     end
                  end

                  if paletteModified[j] == true then
                     break -- pal is now contained (supposedly) in targetPal, so we exit this loop
                  end
               end
            end
         end
      end

      -- if no palette was updated then exit the approximation loop
      if not anyPalUpdated then
         break
      end

      -- 3)
      -- remove palettes that are fully contained in other palette
      palettesAllUnique = removeFullyContainedPalettes(palettesAllUnique)

      -- sort each palette
      for i=1, #palettesAllUnique do
         local pal = palettesAllUnique[i]
         sortPaletteColors(pal)
      end
      -- sort by size
      _sort(palettesAllUnique, sortPalettesAllUnique_func)
   end
end

-- print total palettes sizes
print(string.format("Palettes generated: %d", #palettesAllUnique))
if PRINT_PALETTES_SIZE then
  for i=1, #palettesAllUnique do
     local pal = palettesAllUnique[i]
     print(string.format("[%02d] %d", i, #pal))
  end
end

-- save all palettes from palettesAllUnique in a PNG RGB file
if SAVE_HELPER_FILES then
   local widthImgAllPalsUniq = MAX_COLORS_PER_PALETTE - 1
   local imgPalettesAllUnique = Image(widthImgAllPalsUniq, #palettesAllUnique, ColorMode.RGB)
   for y=0, imgPalettesAllUnique.height - 1 do
     for x=0, imgPalettesAllUnique.width - 1 do
       imgPalettesAllUnique:drawPixel(x, y, colorTransparent)
     end
   end
   for i=1, #palettesAllUnique do
      local pal = palettesAllUnique[i]
      for j=0, #pal - 1 do -- Palette is 0-based internally
         local c = pal:getColor(j)
         imgPalettesAllUnique:drawPixel(j, i-1, c) -- Image pixel positions are 0-based internally
      end
   end
   imgPalettesAllUnique:saveAs(string.format("%s_ALL_pals.png", sprFilename))
end

--  save an image which will help to visualize the colors used by each palette from palettesAllUnique
if SAVE_HELPER_FILES then
   local uniqueColorsArray = getUniqueColorsArray(palettesAllUnique)
   _sort(uniqueColorsArray, sortColorsBy_LUMINANCE_DESC_func)
   local imgMatrix = Image(#uniqueColorsArray, #palettesAllUnique, ColorMode.RGB)
   for y=0, imgMatrix.height - 1 do
     for x=0, imgMatrix.width - 1 do
       imgMatrix:drawPixel(x, y, colorTransparent)
     end
   end
   for i=1, #palettesAllUnique do
      local pal = palettesAllUnique[i]
      for j=0, #pal - 1 do -- Palette is 0-based internally
         local c = pal:getColor(j)
   	  local x = getColorIndex(c, uniqueColorsArray)
         imgMatrix:drawPixel(x-1, i-1, c) -- Image pixel positions are 0-based internally
      end
   end
   imgMatrix:saveAs(string.format("%s_MATRIX_pals.png", sprFilename))
end

-- ###########################################################################################################
if ID_PROC == 0 then
  print(string.format("-- STEP 4.a: Computes how many permutations we need to itereate in order to find the %d palettes.", TARGET_PALETTES))
end

-- create an array of sizes per palette, which will be used as grouping for the permutation algorithm
local sizesGrouping = createSizePaletteGrouping(palettesAllUnique)

if ID_PROC == 0 then
  print(string.format("Sizes Groupings array: %s", getGroupingsAsStringList(sizesGrouping)))
  local totalPerms = computeNumberOfPermutationsWithGroupings(sizesGrouping, 1)
  if totalPerms < 0 then -- in case the compute of total permutations exceeds the biggest supported integer
    totalPerms = math.maxinteger -- 9223372036854775807 in 64bits systems
  end
  print(string.format("Total number of permutations by Brute Force (with grouping): %d", totalPerms))
  if SINGLE_INSTANCE == false then
    -- if #palettesAllUnique >= 3 then 2 first elements will be fixed, else no element is fixed
    local startCalcPermAt = ternary(#palettesAllUnique >= 3, 3, 1)
    local totalPermsPerProc = computeNumberOfPermutationsWithGroupings(sizesGrouping, startCalcPermAt)
    local fixedElems = ternary(#palettesAllUnique >= 3, 2, 0)
    print(string.format("Permutations per instance (with %d fixed elems): %d", fixedElems, totalPermsPerProc))
    local procsFileName = string.format("%s.procs", sprFilename)
    -- when #palettesAllUnique >= 3 we need -1 because first palette is fixed
    local instances = ternary(#palettesAllUnique >= 3, #palettesAllUnique - 1, 1)
    print(string.format("Permutation instances to run: %d", instances))
    local procsFile = io.open(procsFileName, "w")
    procsFile:write(instances)
    procsFile:close()
    -- exit the current execution
    goto _done
  end
end

print(string.format("%02d: -- STEP 4.b: Create up to %d buckets of colors acting as palettes.", ID_PROC, TARGET_PALETTES))

-- 4: I have 4 buckets (arrays) named bucket1, bucket2, bucket3, and bucket4. Given the array palettesAllUnique which contains 
-- Palette objects, where each Palette object contains colors and the size of each palette is given by #palette, and each color 
-- is accessed by method getColor(index), I need to assign each palette's color to a bucket, starting from the first one, 
-- in such a way the accumulative amount of colors per bucket (the occupancy) does not goes beyond 16. If a palette's number of 
-- colors, excluding those colors already in the current bucket, exceed the free space of the bucket then that palette must goes 
-- in the next bucket. It needs to maximize bucket occupancy, minimize number of buckets used.

local bucket1, bucket2, bucket3, bucket4
local allBuckets = nil

-- create an array of accesses to elements of palettesAllUnique
local accesses = {}
for i=1, #palettesAllUnique do
   accesses[i] = i
end

-- Biggest Palette is fixed. Be aware the order palettesAllUnique is sorted
if SINGLE_INSTANCE == false then
  if PALETTES_ORDER_WAY == "ASC" then
    swap(accesses, 1, #accesses) -- this puts biggest palette at the beginning
  end

  -- Do a swap with between N element and ID_PROC + 1 (first element is already fixed), so each process starts its own permutation branch
  if #palettesAllUnique >= 3 then
    local startSwapAt = 2
    -- remember first element in accesses[] is fixed, so ID_PROC won't reach the end
    swap(accesses, startSwapAt, ID_PROC + 1)
  end
end

-- Parameters for the permutations algorithm
-- If SINGLE_INSTANCE = false then start from 3rd element always since the 1st is always fixed and the 2nd is swapped per process
local fromIndex = ternary(SINGLE_INSTANCE == false and #palettesAllUnique >= 3, 3, 1)
local firstGroup = sizesGrouping[fromIndex]
local lastGroup = sizesGrouping[#sizesGrouping]
local toIndex = getGroupEndingIndex(sizesGrouping, firstGroup)

print(string.format("%02d: %s started permutations", ID_PROC, os.date("%c")))

-- Generates all possible permutations of elements from palettesAllUnique by groups of same palette sizes, 
-- using an array of indexes access
generatePermutationsByGroup(accesses, fromIndex, toIndex, sizesGrouping, firstGroup, lastGroup, function (accessArr)

   -- Define the four buckets (first color in each bucket must be the transparent color)
   local bucket1Perm = {colorTransparent}
   local bucket2Perm = {colorTransparent}
   local bucket3Perm = {colorTransparent}
   local bucket4Perm = {colorTransparent}
   local allBucketsPerm = {bucket1Perm, bucket2Perm, bucket3Perm, bucket4Perm}
   local bucketInsertIndex = {2, 2, 2, 2} -- starting insert indexes for buckets, index 1 has color transparent
   local permutationSucceeded = true

   -- Loop through each palette object in the palettesAllUnique array given the current permutation of accessArr   
   for i=1, #palettesAllUnique do
      local pal = palettesAllUnique[accessArr[i]]
      local minOccupancy = MAX_COLORS_PER_PALETTE + 1 -- high starting value
      local minBucketIndex = 1
      local colorsToAdd = nil

      -- Find the bucket with the minimum occupancy so current palette can fit in it
      for j=1, TARGET_PALETTES do
        local bucket = allBucketsPerm[j]
        local occupancy = #bucket -- current bucket's occupancy
        local colorsToAddInLoop = {}
        local toAddIndex = 1

        -- Calculate the occupancy of the bucket given the current palette
        for k=0, #pal-1 do
           local color = pal:getColor(k) -- Palette is 0-based internally
           if not containsColor(bucket, color) then
              occupancy = occupancy + 1
              colorsToAddInLoop[toAddIndex] = color
              toAddIndex = toAddIndex + 1
           end
           -- if we exceeded the max occupancy allowed then stop trying
           if occupancy > MAX_COLORS_PER_PALETTE then
              break -- try next bucket
           end
        end

        -- keep track of minimum bucket size and number of colors to add for the current palette
        if occupancy < minOccupancy then
           minOccupancy = occupancy
           minBucketIndex = j
           colorsToAdd = colorsToAddInLoop
         end
      end

      -- If current palette's colors (except those already in the bucket) fit in minBucket, add the missing colors
      if minOccupancy <= MAX_COLORS_PER_PALETTE then
         local minBucket = allBucketsPerm[minBucketIndex]
         for k=1, #colorsToAdd do
            local color = colorsToAdd[k]
            local insInd = bucketInsertIndex[minBucketIndex]
            minBucket[insInd] = color
            bucketInsertIndex[minBucketIndex] = insInd + 1
         end
      else
         --print(string.format("PALETTE %d size %d DIDN'T FIT IN ANY BUCKET", i, #pal))
         permutationSucceeded = false
         break -- exit the current permutation
      end
   end

   -- Checks if permutation succeeded for this coroutine
   if permutationSucceeded then
      print(string.format("%02d: PERMUTATION FOUND!", ID_PROC))
      bucket1 = bucket1Perm
      bucket2 = bucket2Perm
      bucket3 = bucket3Perm
      bucket4 = bucket4Perm
      allBuckets = allBucketsPerm
      -- order elems in palettesAllUnique[] according the indexes from accessArr
      local reordered = {}
      for i = 1, #accessArr do
         reordered[i] = palettesAllUnique[accessArr[i]]
      end
      for i = 1, #palettesAllUnique do
         palettesAllUnique[i] = reordered[i]
      end
   end

   return permutationSucceeded
end)

print(string.format("%02d: %s finished permutations", ID_PROC, os.date("%c")))

if allBuckets == nil then
   print(string.format("%02d: NO PERMUTATION SUCCEEDED. TOO MUCH DIFFERENT PALETTES TO FIT INTO %d BUCKETS.", ID_PROC, TARGET_PALETTES))
   goto _done -- terminates the script immediately and without warning
end

-- Print size of each bucket only those having more than 1 palette
for i=1, TARGET_PALETTES do
   local bucket = allBuckets[i]
   if #bucket > 1 then
      print(string.format("%02d: Bucket %d: %d", ID_PROC, i, #bucket))
   end
end

-- Sort each bucket by color in ASC order
for i=1, TARGET_PALETTES do
   local bucket = allBuckets[i]
   -- only buckets having more than the transparent color
   if #bucket > 1 then
      _sort(bucket, function(c1, c2)
         -- color with alpha is the transparent color so it must be the set at first position
         if c1.alpha == 0 then
            return true
         elseif c2.alpha == 0 then
            return false
         else
            if c1.red < c2.red then
               return true
            elseif c1.red == c2.red then
               if c1.green < c2.green then
                  return true
               elseif c1.green == c2.green then
                  return c1.blue < c2.blue
               end
            end
            return false
         end
      end)
   end
end

local function slice (tbl, first, last)
  local sliced = {}
  for i = first or 1, last or #tbl, 1 do
    sliced[#sliced+1] = tbl[i]
  end
  return sliced
end

local function sliceAndAddFirst (tbl, first, last, elem)
  local sliced = {elem}
  for i = first or 1, last or #tbl, 1 do
    sliced[#sliced+1] = tbl[i]
  end
  return sliced
end

-- if MAX_COLORS_PER_PALETTE > 16 then we need to split first bucket into TARGET_PALETTES buckets
-- NOTE: only tested with MAX_COLORS_PER_PALETTE = 60
if MAX_COLORS_PER_PALETTE > MAX_COLORS_PER_PALETTE_FOR_MD_RGB then
   local pivotBucket = allBuckets[1]
   if #pivotBucket > MAX_COLORS_PER_PALETTE_FOR_MD_RGB then
      -- first bucket will always have colorTransparent as first element
      bucket1 = slice(pivotBucket, 1, MAX_COLORS_PER_PALETTE_FOR_MD_RGB)
      if TARGET_PALETTES > 1 then
        if #pivotBucket < (MAX_COLORS_PER_PALETTE_FOR_MD_RGB * 2) then
      	  bucket2 = sliceAndAddFirst(pivotBucket, MAX_COLORS_PER_PALETTE_FOR_MD_RGB + 1, #pivotBucket, colorTransparent)
        else
          bucket2 = sliceAndAddFirst(pivotBucket, MAX_COLORS_PER_PALETTE_FOR_MD_RGB + 1, (MAX_COLORS_PER_PALETTE_FOR_MD_RGB * 2) - 1, colorTransparent)
          if TARGET_PALETTES > 2 then
            if #pivotBucket < (MAX_COLORS_PER_PALETTE_FOR_MD_RGB * 3) then
      	      bucket3 = sliceAndAddFirst(pivotBucket, (MAX_COLORS_PER_PALETTE_FOR_MD_RGB * 2), #pivotBucket, colorTransparent)
      	    else
      	      bucket3 = sliceAndAddFirst(pivotBucket, (MAX_COLORS_PER_PALETTE_FOR_MD_RGB * 2), (MAX_COLORS_PER_PALETTE_FOR_MD_RGB * 3) - 2, colorTransparent)
              if TARGET_PALETTES > 3 then
      	        bucket4 = sliceAndAddFirst(pivotBucket, (MAX_COLORS_PER_PALETTE_FOR_MD_RGB * 3) - 1, #pivotBucket, colorTransparent)
              end
      	    end
          end
        end
      end
   end
   allBuckets = {bucket1, bucket2, bucket3, bucket4}
end

-- ###########################################################################################################
print(string.format("%02d: -- STEP 5: Generate Final Palette.", ID_PROC))

-- 5: Generate the final Palette
local finalPaletteSize = 0
for i=1, TARGET_PALETTES do
   local bucket = allBuckets[i]
   -- only buckets having more than the transparent color
   if #bucket > 1 then
      finalPaletteSize = finalPaletteSize + #bucket
   end
end
local finalPalette = Palette(finalPaletteSize)
local iForFinalPal = 0 -- Palettes are 0-based internally
for i=1, TARGET_PALETTES do
   local bucket = allBuckets[i]
   -- only buckets having more than the transparent color
   if #bucket > 1 then
      for j=1, #bucket do
         local c = bucket[j]
         finalPalette:setColor(iForFinalPal, c)
         iForFinalPal = iForFinalPal + 1
      end
   end
end
finalPalette:resize(finalPaletteSize)

-- save the final Palette as pal file
if SAVE_PALETTE_FILES then
   finalPalette:saveAs(string.format("%s_pal.pal", sprFilename))
end

-- save the final Palette as a PNG RGB image
if SAVE_PALETTE_FILES then
   local imgFinalPal = Image(MAX_COLORS_PER_PALETTE_FOR_MD_RGB, 4, ColorMode.RGB)
   for y=0, 4-1 do
     for x=0, MAX_COLORS_PER_PALETTE_FOR_MD_RGB - 1 do
       imgFinalPal:drawPixel(x, y, colorTransparent)
     end
   end
   for i=1, TARGET_PALETTES do
      local bucket = allBuckets[i]
      -- only buckets having more than the transparent color
      if #bucket > 1 then
         for j=1, #bucket do
            local c = bucket[j]
            imgFinalPal:drawPixel(j-1, i-1, c) -- Image pixel positions are 0-based internally
         end
      end
   end
   imgFinalPal:saveAs(string.format("%s_pal.png", sprFilename))
end

-- ###########################################################################################################
print(string.format("%02d: -- STEP 6: Add the Palettes at the top left and turn the image into a new RGB image.", ID_PROC))
-- 6: Add the palette at the top left and turn the image into an RGB format.

-- Create a new image
local offsetY = 32 -- first 32 rows contains the RGB palettes
local newSpriteWidth = spriteWidth
local newSpriteHeight = spriteHeight + offsetY
local newImg = Image(newSpriteWidth, newSpriteHeight, ColorMode.RGB)

-- Set all Image pixels with transparent color
for y=0, newSpriteHeight - 1 do
  for x=0, newSpriteWidth - 1 do
    newImg:drawPixel(x, y, colorTransparent)
  end
end

-- Add the palettes as 8x8 rows
local currentY = 0
for i=1, TARGET_PALETTES do
  local currentX = 0
  local bucket = allBuckets[i]
  if #bucket > 1 then -- only buckets having more than the transparent color
    for j=1, #bucket do
      local c = bucket[j]
      printColor8x8InImage(newImg, c, currentX, currentY)
      currentX = currentX + 8
    end
  end
  currentY = currentY + 8
end

-- Copy the original Sprite's image into the new image
local allImg = sprite.cels[1].image
local sprPalette = sprite.palettes[1]

for y=0, spriteHeight - 1 do
  for x=0, spriteWidth - 1 do
    local pi = allImg:getPixel(x, y) -- this is an index pointing to the Sprite's Palette
    local c = sprPalette:getColor(pi)
    -- draw the color in the new image
    newImg:drawPixel(x, y + offsetY, c)
  end
end

-- do a random wait in seconds to avoid the next save statement overwrites an existing solution file
wait(math.random(1,4))
if not fileExists(string.format("%s_RGB.png", sprFilename)) then
  newImg:saveAs(string.format("%s_RGB.png", sprFilename))
  print(string.format("%02d: ####### IMAGE SAVED. #######", ID_PROC))
end

::_done::