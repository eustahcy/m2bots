import os
import sys
from PIL import Image
from PIL import ImageDraw
import random

rectList = []
for line in open("index"):
	path = line.split()[1]
	if path[-1] == os.linesep:
		path = path[:-1]

	for line2 in open(path + "/" + "Setting.txt"):
		tokens = line2.split()
		if not tokens:
			continue

		if tokens[0] == "BasePosition":
			x, y = [int(item) / 100 for item in tokens[1:3]]
		elif tokens[0] == "MapSize":
			w, h = [int(item) * 256 for item in tokens[1:3]]

	rectList.append((path, x, y, x+w, y+h))

#x = int(sys.argv[1])
#y = int(sys.argv[2])

alreadyChecked = []

scale = 0.25

image = Image.new("RGB", (int(30000*scale), int(30000*scale)), "white")
pixels = image.load()
# img_draw = ImageDraw.Draw(image)


for map1 in rectList:
	x = int(map1[1] * scale)
	y = int(map1[2] * scale)
	x_end = int(map1[3] * scale)
	y_end = int(map1[4] * scale)
	color = [random.randint(70,200), random.randint(70,200), random.randint(70,200)]
		
	for i in range(x, x_end):
		for j in range(y, y_end):
			try:
				pixels[i, j] = (color[0], color[1], color[2])
			except:
				print "map: %s, out of range: x: %d, y: %d" % (map1[0], i, j)
				break
				
	ImageDraw.Draw(
		image  # Image
	).text(
		(x, y),  # Coordinates
		map1[0],  # Text
		(0, 0, 0)  # Color
	)
	
	# for checkMap in rectList:
		# if map1[0] == checkMap[0]:
			# continue
			
		# mapsName = "%s:%s" % (map1[0], checkMap[0])
		# mapsNameReversed = "%s:%s" % (checkMap[0], map1[0])
		
		# if mapsName in alreadyChecked or mapsNameReversed in alreadyChecked:
			# continue
			
		# if x >= checkMap[1] and y >= checkMap[2] and x <= checkMap[3] and y<= checkMap[4]:
			# print map1[0] + " WITH " + checkMap[0]
			# alreadyChecked.append("%s:%s" % (map1[0], checkMap[0]))
		
		# if x_end >= checkMap[1] and y_end >= checkMap[2] and x_end <= checkMap[3] and y_end <= checkMap[4]:
			# print map1[0] + " WITH " + checkMap[0]
			# alreadyChecked.append("%s:%s" % (map1[0], checkMap[0]))
image.save(r"D:\Mt2\Thrl\Metin2009\Serverfiles\share\locale\english\map\czesc.png", 'png')