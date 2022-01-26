**SystemVerifier** is a small tool to help when sketching installations consisting of multiple components intricately connected with each other.

![](https://github.com/artuomsci/SystemVerifier/blob/main/Example.png)

**Motivation**

One of the assignments I had at my job was to maintain a list of hardware components comprising the interactive virtual training system. Some of the components had to be replaced over time as they became obsolete, newer components were added as new features became available. Sometimes customers wanted systems tailored specifically for their needs wich led to a redesign from the ground up.

I wasn't really excited to do it! It was tedious, repetitive and error prone. Multiple devices connected to each other through a limited number of sockets of certain types by means of cables having the same socket types but different gender. It all starts looking intimidating as soon as you hit the count 4 for such an innocent thing as a video monitor.

As a programmer I was used to compiler checking my every move. So, the natural step was to try to automate the chore like the compiler does. Looking at the above problem a bit closer it's not hard to see the patterns. Devices and connecting them cables are like functions with arguments for sockets. The whole setup is like expression consisting of multiple functions combined with respect to input/output types. The valid expression means correct system design, at least from the topological view.

**Dependencies**

- Qt 4/5
    
**Building**

Create a build folder /bin. Compile program. Run from /bin folder.
