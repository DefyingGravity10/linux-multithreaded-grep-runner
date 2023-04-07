# linux-multithreaded-grep-runner

<h2><strong>Overview</strong></h2>
This project is my implementation of a Linux-based parallelized <code>grep</code> runner in C. Specifically, it involves multithreading where <code>1-8</code> "workers" could be used. 
The project was submitted as a requirement for the <strong>CS 140 (Operating Systems)</strong> Course of the Department of Computer Science in the University of the Philippines - Diliman during the First Semester of A.Y. 2022 - 2023.

<h2><strong>How to Use the Project</strong></h2>
<h3>Pre-requisite</h3>
Ensure that you will be running the program on a <strong>Linux OS</strong>. Do bear in mind that this program was specifically written to run in a machine running <code>Ubuntu 22.04</code>. Thus, it will be ideal if this were used.

<h3>Running the Program</h3>
This program comes with two files: <code>single.c</code> & <code>multithreaded.c</code>. The former performs <code>grep</code> with a single "worker", while the latter performs the same operation with <code>N</code> (1-8) "workers" as specified by the user (see below). Note that only <strong>one</strong> of the programs will be executed at a time.<br/><br/>

After placing the C files in the desired location, perform the following steps:<br/><br/>
<strong>single.c</strong>
<ol>
  <li>Compile the program through the command <code>gcc single.c -o single</code>.</li>
  <li>Run the program through the command <code>./single (X) (Root of Directory Tree to Search) (Search String to be used by grep)</code></li> In this case, <code>X</code> could be set to any number. 
</ol><br/>

<strong>multithreaded.c</strong>
<ol>
  <li>Compile the program through the command <code>gcc -pthread multithreaded.c -o multithreaded</code>.</li>
  <li>Run the program through the command <code>./multhreaded (N) (Root of Directory Tree to Search) (Search String to be used by grep)</code></li> In this case, <code>X</code> could be set to any within the range of <code>1-8</code> (inclusive). 
</ol>
