def partend(start,string):
    output = ""
    for x in range(len(string)-start):
        output = output+string[x+start]
    return output
