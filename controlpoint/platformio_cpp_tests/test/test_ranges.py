
OUT_MIN=0
OUT_MAX=100
def map_val(v, in_min, in_max, out_min, out_max):
    return ( v - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

def adjust(v,low,hi):
    if low < hi:
        return map_val(v,OUT_MIN,OUT_MAX,low-1,hi)
    else:
        return map_val(v,OUT_MIN,OUT_MAX,low+1,hi)

if __name__ == "__main__":
    vals = [0,1,5,10,11,20,50,60,90,95,100];
    for v in vals:
        print("%d --> %d" % (v,adjust(v,0,7)))
    print "-----"
    for v in vals:
        print("%d --> %d" % (v,adjust(v,18,14)))        