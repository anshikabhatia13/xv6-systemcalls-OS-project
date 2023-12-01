// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "console.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void consputc(int);

static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}

void
panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for(;;)
    ;
}

//PAGEBREAK: 50
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

struct {
  char buf[INPUT_BUF];
  uint r;  // Read index (first index of buf)
  uint w;  // Write index (last index of buf)
  uint e;  // Edit index (current index)
  uint rightmost; // the first empty char in the line
} input;

char charsToBeMoved[INPUT_BUF];  // temporary storage for input.buf in a certain context



/*Q3 Changes made to add a struct that will hold history keeping metrics */

struct {
  uint indLastCmd; 
   
  uint lenofeachCmdStr[MAX_HISTORY]; // len(command string)
  
  int numofMemHistCommands; 
  
  char actualCmdStr[MAX_HISTORY][INPUT_BUF]; // actual command strings -
  
  
  int currentHistory; // current history view -> displacement from the last command index 
  
} hisBuffArr;

char oldBufferArray[INPUT_BUF]; // details of the command that we were typing before accessing the history
uint lenOfOldBuffer;

char buf2[INPUT_BUF];

/*Changes end */



#define C(x)  ((x)-'@')  // Control-x

static void
cgaputc(int c, int flag)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  switch(c) {
    case '\n':
      pos += 80 - pos%80;
      break;
    case BACKSPACE:
      if(pos > 0) --pos;
      break;
    case LEFT_ARROW:
      if(pos > 0) --pos;
      break;
    case RIGHT_ARROW:
      break;
    default:
      crt[pos++] = (c&0xff) | 0x0700;  // black on white
  }

  if(pos < 0 || pos > 25*80)
    panic("pos under/overflow");

  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);

  if (c != LEFT_ARROW && c != RIGHT_ARROW && flag != 1) crt[pos] = ' ' | 0x0700;
}

void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }

  switch (c) {
    case BACKSPACE:
      // uartputc prints to Linux's terminal
      uartputc('\b'); uartputc(' '); uartputc('\b');  // uart is writing to the linux shell
      break;
    case LEFT_ARROW:
      uartputc('\b');
      break;
    case RIGHT_ARROW:
      if (input.e < input.rightmost) {
        uartputc(input.buf[input.e % INPUT_BUF]);
        cgaputc(input.buf[input.e % INPUT_BUF], 1);
        input.e++;
      }
      break;
    default:
      uartputc(c);
  }
  if (c != RIGHT_ARROW) cgaputc(c, 0);
}

/*
Store input.buf into charsToBeMoved (to use later)
Called when a new key is pressed and the cursor is not at EOL
*/
void copyCharsToBeMoved() {
  for (uint i = 0; i < (uint)(input.rightmost - input.r); i++) {
    charsToBeMoved[i] = input.buf[(input.e + i) % INPUT_BUF];
  }
}

/*
Shift input.buf (backend) to the right by one unit and print the same on-screen (front-end)
Called when a new key is pressed and the cursor is not at EOL
*/
void rightShiftBuffer() {
  uint n = input.rightmost - input.e;
  for (uint i = 0; i < n; i++) {
    input.buf[(input.e + i) % INPUT_BUF] = charsToBeMoved[i];
    consputc(charsToBeMoved[i]);
  }

  // reset charsToBeMoved for further use
  for(uint i = 0; i < INPUT_BUF; i++) {
    charsToBeMoved[i] = '\0';
  }
  // return the caret to its correct position
  int i = n;
  while (i--) {
    consputc(LEFT_ARROW);    
  }
}

/*
Shift input.buf (backend) to the left by one unit and print the same on-screen (front-end)
Called when a BACKSPACE is pressed and the cursor is not at EOL
*/
void leftShiftBuffer() {
  /**
   * For Ex: Input is abcdef and cursor is b/w c and d.
   * @cursor (display) : @pos in cgaputc
   */
  uint n = input.rightmost - input.e;
  consputc(LEFT_ARROW); // cursor (display) is b/w b and c
  input.e--; // set the backend part of cursor to the final correct position

  // abcdef -> abdeff
  for (uint i = 0; i < n; i++) {
    input.buf[(input.e + i) % INPUT_BUF] = input.buf[(input.e + i + 1) % INPUT_BUF];
    consputc(input.buf[(input.e + i + 1) % INPUT_BUF]);
  }
  // cursor (display) is b/w f and f

  input.rightmost--; // set input.rightmost to the final correct position
  consputc(' '); // delete the last char in line and advance cursor (display) by 1
  
  // set the cursor (display) to the correct position
  int i = n + 1;
  while (i--){
    consputc(LEFT_ARROW); // shift the caret back to the left
  }

  // at this point, the cursor (display) i.e. pos is in sync with input.e
}

/*
  Trigger interupt to handle console
*/
void
consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;
  uint tempIndex;
  acquire(&cons.lock);
  while((c = getc()) >= 0){
    switch(c){
      case C('P'):  // Process listing.
        doprocdump = 1;   // procdump() locks cons.lock indirectly; invoke later
        break;
      case C('U'):  // Kill line.
        if (input.rightmost > input.e) { // caret isn't at the end of the line
          uint numtoshift = input.rightmost - input.e;
          uint placestoshift = input.e - input.w;
          uint i;
          for (i = 0; i < placestoshift; i++) {
            consputc(LEFT_ARROW);
          }
          memset(buf2, '\0', INPUT_BUF);
          for (i = 0; i < numtoshift; i++) {
            buf2[i] = input.buf[(input.w + i + placestoshift) % INPUT_BUF];
          }
          for (i = 0; i < numtoshift; i++) {
            input.buf[(input.w + i) % INPUT_BUF] = buf2[i];
          }
          input.e -= placestoshift;
          input.rightmost -= placestoshift;
          for (i = 0; i < numtoshift; i++) { // repaint the chars
            consputc(input.buf[(input.e + i) % INPUT_BUF]);
          }
          for (i = 0; i < placestoshift; i++) { // erase the leftover chars
            consputc(' ');
          }
          for (i = 0; i < placestoshift + numtoshift; i++) { // move the caret back to the left
            consputc(LEFT_ARROW);
          }
        }
        else { // caret is at the end of the line -                                       ( deleting everything from both screen and inputbuf)
          while(input.e != input.w &&
                input.buf[(input.e - 1) % INPUT_BUF] != '\n'){
            input.e--;
            input.rightmost--;
            consputc(BACKSPACE);
          }
        }
        break;
      case C('H'): case '\x7f':  // Backspace
        if (input.rightmost != input.e && input.e != input.w) { // caret isn't at the end of the line
          leftShiftBuffer();
          break;
        }
        if(input.e != input.w){ // caret is at the end of the line - deleting last char
          input.e--;
          input.rightmost--;
          consputc(BACKSPACE);
        }
        break;
        /*Q3 ADD NEW CASES FOR NAVIGATION*/
        
        
      case UP_ARROW:
      
       if (hisBuffArr.currentHistory < hisBuffArr.numofMemHistCommands-1 ) // if yes then still more historical commands to display
       { 
          rmCurrLinefromConsole(); // clears the current console line
          
          if (hisBuffArr.currentHistory == -1) // if no history, moves the chars to "old" commands' buffer
              charstoOlsBuff();
              
          clrInpBuff();// any content on current input buffer that contains history commands to be printed is erased
          
          hisBuffArr.currentHistory++;// incremented to move next cmd in history
          
          tempIndex = (hisBuffArr.indLastCmd + hisBuffArr.currentHistory) % MAX_HISTORY; // can be used to access specific command from the history 
          
          copyBufftoConsole(hisBuffArr.actualCmdStr[ tempIndex], hisBuffArr.lenofeachCmdStr[tempIndex]);
          //temp index is letting the access of all command strings including the current
          
          copybuftoInpBuff(hisBuffArr.actualCmdStr[ tempIndex], hisBuffArr.lenofeachCmdStr[tempIndex]);
          // last command(history) is also added to the history buffer now
        }
        break;
        
        // down arrow case: 
      case DOWN_ARROW:
        switch(hisBuffArr.currentHistory)
        {
        
          case -1:
            
            break; // no history available do nothing

          case 0:  // if its the first cmd in history, 
            rmCurrLinefromConsole();
            
            copybuftoInpBuff(oldBufferArray, lenOfOldBuffer);// current cmd added to buffer n can be navigated now
            
            copyBufftoConsole(oldBufferArray, lenOfOldBuffer);// current can be shown on screen too
            
            hisBuffArr.currentHistory--; // every downward move curr history index decreases in comparison from start at the arrow's cmd
            
            break;

          default:
            rmCurrLinefromConsole();
            
            hisBuffArr.currentHistory--;
            // else simple navigation with decreasing index current to buffer screen and to history
            
            tempIndex = (hisBuffArr.indLastCmd + hisBuffArr.currentHistory)%MAX_HISTORY;
            
            copyBufftoConsole(hisBuffArr.actualCmdStr[ tempIndex]  , hisBuffArr.lenofeachCmdStr[tempIndex]);
            
            copybuftoInpBuff(hisBuffArr.actualCmdStr[ tempIndex]  , hisBuffArr.lenofeachCmdStr[tempIndex]);
            
            break;
        }
        break;
        //Q3 changes end
        
        
        /*handling simple cases of left and right arrow not asked in quess*/
      case LEFT_ARROW:
      
        if (input.e != input.w) {
        
          input.e--; // e is for edit index, so on left arrow we can simply decrease the index and move to the previous
          consputc(c);
        }
        break;
      case RIGHT_ARROW:
      
        consputc(RIGHT_ARROW); // constputc is already in xv6 that defines how to behave during RIGHT ARROW
        
        break;
      
      
      default:
        if(c != 0 && input.e-input.r < INPUT_BUF){
          c = (c == '\r') ? '\n' : c;
          if (input.rightmost > input.e) { // caret isn't at the end of the line
            copyCharsToBeMoved();
            input.buf[input.e % INPUT_BUF] = c;
            input.e++;
            input.rightmost++;
            consputc(c);
            rightShiftBuffer();
          }
          else {
            input.buf[input.e % INPUT_BUF] = c;
            input.e++;
            input.rightmost = input.e - input.rightmost == 1 ? input.e : input.rightmost;
            consputc(c);
          }
          if(c == '\n' || c == C('D') || input.rightmost == input.r + INPUT_BUF){
          
            saveHistoricalCommands(); // add here to save commands in his buffer
            input.w = input.rightmost;
            wakeup(&input.r);
          }
        }
        break;
      }
  }
  release(&cons.lock);
  if(doprocdump) {
    procdump();  // now call procdump() wo. cons.lock held
  }
}


// Q3 Again start changing- helper functions...........


void 
rmCurrLinefromConsole(void) 
{
    int length = input.rightmost - input.r;
    while (length--) 
    {
      consputc(BACKSPACE);
    }
}


void 
charstoOlsBuff(void) 
{
    lenOfOldBuffer = input.rightmost - input.r;
    for (uint i = 0; i < lenOfOldBuffer; i++) 
    {
      oldBufferArray[i] = input.buf[(input.r + i) % INPUT_BUF];
    }
}


void 
clrInpBuff()
{
  input.rightmost = input.r;
  input.e = input.r;
}


void 
copyBufftoConsole(char* bufToPrintOnScreen, uint length)
{
  uint i = 0;
  while(length--) 
  {
    consputc(bufToPrintOnScreen[i]);
    i++;
  }
}

void copybuftoInpBuff(char * bufToSaveInInput, uint length)
{
  for (uint i = 0; i < length; i++) 
  {
    input.buf[(input.r + i) % INPUT_BUF] = bufToSaveInInput[i];
  }

  input.e = input.r + length;
  input.rightmost = input.e;
}


void 
saveHistoricalCommands()

{
  uint len = input.rightmost - input.r - 1; // Calculate the length of the command
  
  
  if (len == 0) return;  // If the command is empty, return without saving
  
  hisBuffArr.currentHistory = -1; // Set the current history index to -1 (indicating no current history selection)
  
  if (hisBuffArr.numofMemHistCommands < MAX_HISTORY) 
  {
    hisBuffArr.numofMemHistCommands++;// Increment the count of stored historical commands if within capacity
  }
  
  hisBuffArr.indLastCmd = (hisBuffArr.indLastCmd - 1) % MAX_HISTORY; // Update the index for the last command in the circular history buffer
  
  hisBuffArr.lenofeachCmdStr[hisBuffArr.indLastCmd] = len;// Store the length of the saved command in the length array
  
  for (uint i = 0; i < len; i++) 
  { 
    hisBuffArr.actualCmdStr[hisBuffArr.indLastCmd][i] =  input.buf[(input.r + i) % INPUT_BUF];// Copy each character of the command to the historical command buffer
    
  }
}


int 
AccessHistory(char *buffer, int historyId) 
{
  if (historyId < 0 || historyId > MAX_HISTORY - 1)
    return 2; // Check if the historyId is out of bounds
    
  if (historyId >= hisBuffArr.numofMemHistCommands)
    return 1;// Check if the requested historyId is beyond the number of stored historical commands
    
  memset(buffer, '\0', INPUT_BUF); // Clear the buffer before copying the historical command
  
  int tempIndex = (hisBuffArr.indLastCmd + historyId) % MAX_HISTORY; // Calculate the index in the circular history buffer
  
  memmove(buffer, hisBuffArr.actualCmdStr[tempIndex], hisBuffArr.lenofeachCmdStr[tempIndex]);  // Copy the historical command to the provided buffer
  
  return 0;
}



/*Q3 changes send here */

int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(input.r == input.w){
      if(myproc()->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void
consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);

  hisBuffArr.numofMemHistCommands=0;
  hisBuffArr.indLastCmd=0;
}
