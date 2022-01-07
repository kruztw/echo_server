import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties

myfont = FontProperties(size=40)

 
elapsed_time = [0.5, 2, 28, 326, 2563, 24879]

connections = [0, 1, 2, 3, 4, 5]

plt.figure(figsize=(15,10),dpi=100,linewidth = 2)
plt.plot(connections, elapsed_time,'o-',color = 'b', label="elpased time")


plt.xticks(fontsize=20)
plt.yticks(fontsize=20)

plt.xlabel("log10(client number)", fontsize=30, labelpad = 15)
plt.ylabel("ms", fontsize=30, labelpad = 20)

plt.legend(loc = "best", fontsize=20)
#plt.xticks([1, 2, 3, 4])

for c, t in zip(connections, elapsed_time):
    label = "{:}".format(t)
    plt.annotate(label, (c,t), textcoords="offset points",xytext=(-10,10),ha='center')

plt.show()

