// NDRtoSN.c      
//    
// Converts .ndr file into .lsn or .hsn file v1.2
// Stores tables of names for places and transitions as comments
// Processes inhibitor and priority arcs 
// Processes transition substitution labels
//
// Usage: NDRtoSN file1.ndr file2.lsn
//        NDRtoSN file1.ndr file2.hsn
//
// Compile: gcc -o NDRtoSN NDRtoSN.c al2.c
//
// Uses abstract lists al2.h and al2.c from https://github.com/dazeorgacm/ts
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#include "al2.h"

void in_l2_tail(struct l2 ** pq, struct l2 * e)
{
  struct l2 *e1, *head, *tail;

  if(*pq==NULL) // empty queue
  {
    *pq=e;
    e->next=e;
    e->prev=e;
  }
  else // non empty queuue
  {
    head=*pq;
    tail=head->prev;
    tail->next=e;
    e->prev=tail;
    e->next=head;
    head->prev=e;
  }
} /* in_l2_head */

void in_l2_order(struct l2 ** pq, struct l2 * e,int (*compare_content)(void *,void *))
{
  struct l2 *curr, *prev, *head;
 
  if(*pq==NULL) // empty queue
  {
    *pq=e;
    e->next=e;
    e->prev=e;
  }
  else
  {
    curr=*pq;
    head=*pq;
    do
    {
      if( (*compare_content)(e->content, curr->content) == -1 )
      {
        prev=curr->prev;
        prev->next=e;
        curr->prev=e;
        e->prev=prev;
        e->next=curr;
        if(curr==head) *pq=e;
        return;
      }
      curr=curr->next;
    }while(curr!=head);
    prev=curr->prev;
    prev->next=e;
    curr->prev=e;
    e->prev=prev;
    e->next=curr;
  }
  
} /* in_l2 */

struct l2 * from_l2_head( struct l2 ** pq )
{
  struct l2 *e, *head, *tail;

  if( *pq == NULL ) return NULL; // no elements

  e=*pq;
  head=e->next;
  tail=e->prev;

  if(e == e->next) // 1 element
  {
    *pq=NULL;
  }
  else // more than 1 elements
  {
    *pq=head;
    head->prev=tail;
    tail->next=head;
  } 
  return e; 
}/* from_l2_head */

struct l2 * from_l2( struct l2 ** pq, void *sample, int (*find_content)(void *,void *))
{
  struct l2 *curr, *head, *prev, *next, *e;

  curr=*pq;
  head=*pq;
  e=NULL;

  if(curr==NULL) return NULL;
  
  do
  {
    if( (*find_content)(sample, curr->content) == 1 )
    {
      e=curr;
      break;
    }
    curr=curr->next;
  }while(curr!=head);
  if(e==NULL) return NULL;

  if(e==e->next)
    *pq=NULL;
  else
  {
    next=e->next;
    prev=e->prev;
    prev->next=next;
    next->prev=prev;
    if(e==head) *pq=e->next;
  }
  return e;
} /* from_l2 */

void print_l2( struct l2 *q, void (*print_content)(void *) )
{
  struct l2 * e = q;
  struct l2 * head = q;

  if(e==NULL)return;
  do
  {
    (*print_content)(e->content);
    e=e->next;
  }while(e!=head);
  printf("\n");
}/* print_l2 */

void print_back_l2( struct l2 *q, void (*print_content)(void *) )
{
  struct l2 *e, *tail;

  if(e==NULL)return;
  
  e = q->prev;
  tail = q->prev;

  do
  {
    (*print_content)(e->content);
    e=e->prev;
  }while(e!=tail);
  printf("\n");
}/* print_back_l2 */


#define __MAIN__

//#define MAXINPSTRLEN 16384
//#define MAXFILENAME 256
#define MAXSTRLEN 1025
#define FILENAMELEN 256

#define nINIT 1024
#define mINIT 1024
#define lINIT 1024
#define atpINIT 2048
#define aptINIT 2048
#define attINIT 1024
#define namesINIT 16384

#define nDELTA 1024
#define mDELTA 1024
#define lDELTA 1024
#define atpDELTA 2048
#define aptDELTA 2048
#define attDELTA 512
#define namesDELTA 8192

#define NDR 1
#define NET 2

static char str[ MAXSTRLEN + 1 ]; /* line buffer */
 
static int n, m, l, maxn, maxm, maxl; /* net size: trs, pls, labels */
static int *tn, fat; /* trs */
static int *pn, fap; /* pls */
static int *mu;      /* marking */

static int *tl, *tltn, fatl; /* trs labels */
 
static char *names; /* all the names */
static int fnames, maxnames;
static int netname=-1;
 
static int *atpp, *atpt, *atpw; /* arcs t->p */
static int *aptp, *aptt, *aptw; /* arcs p->t */
static int *att1, *att2;        /* arcs t->t */
static int fatp, fapt, maxatp, maxapt;
static int fatt, maxatt;

static char HSN_prefix[]="{*HSN(";
static int HSN_prefix_length=6;

void SwallowSpace( char * str, int *i )
{
  
  while( ( str[(*i)]==' ' || str[(*i)]==0xa || str[(*i)]==0xd || str[(*i)]==0x9 ) && str[(*i)]!='\0'  ) (*i)++;

} /* SwallowSpace */

int IsSpace( char * str, int i )
{
   
  if( str[i]==' ' || str[i]==0xa || str[i]==0xd || str[i]==0x9 || str[i]=='\0' )
    return( 1 );
  else
    return( 0 );
  
} /* IsSpace */
	
void GetName( int *i, int *j )
{
 int state;

 if( str[(*i)]=='{' )
 {
   state=1;
   while( str[(*i)]!='\0' && state && str[(*i)]!=0xa && str[(*i)]!=0xd ) 
   {
    names[ (*j)++ ]=str[ (*i)++ ]; 
    if( str[(*i)-1]=='}' && state==1 ) state=0; else
      if( str[(*i)-1]=='\\' && state==2 ) state=1; else
        if( str[(*i)-1]=='\\' && state==1 ) state=2; else
	  if( str[(*i)-1]=='}' && state==2 ) state=1; else 
	    if( state==2 ) state=1;
   }
   names[ (*j)++ ]='\0';
 }
 else
 {
   while( str[(*i)]!=' ' && str[(*i)]!='\0' && str[(*i)]!=0xa && str[(*i)]!=0xd && str[(*i)]!=0x9 && str[(*i)]!='*' && str[(*i)]!='?')
    names[ (*j)++ ]=str[ (*i)++ ];
   names[ (*j)++ ]='\0';
 }

} /* GetName */


void ExpandNames()
{
  char * newnames;

  if( fnames+MAXSTRLEN > maxnames )
  { 
    maxnames+=namesDELTA;
    newnames = (char*) realloc( names, maxnames );
    if( newnames==NULL ) { printf( "*** not enough memory (ExpandNames)\n" ); exit(3); }
    else names=newnames;
  }

} /* ExpandNames */

void ExpandP()
{
  int *newpn, *newmu;

  if( m >= maxm - 2 ) 
  {
    maxm+=mDELTA;
    newpn = (int*) realloc( pn, maxm * sizeof(int) );
    newmu = (int*) realloc( mu, maxm * sizeof(int) );
    if( newpn==NULL || newmu==NULL ) 
      { printf( "*** not enough memory (ExpandP)\n" ); exit(3); }
    else 
      { pn=newpn; mu=newmu; }
  }

} /* ExpandP */

void ExpandT()
{
  int *newtn;

  if( n >= maxn - 2 ) 
  {
    maxn+=nDELTA;
    newtn = (int*) realloc( tn, maxn * sizeof(int) );
    if( newtn==NULL ) 
      { printf( "*** not enough memory (ExpandT)\n" ); exit(3); }
    else 
      { tn=newtn; }
  }
  
} /* ExpandT */

void ExpandTL()
{
  int *newtl, *newtltn;

  if( l >= maxl - 2 ) 
  {
    maxl+=lDELTA;
    newtl = (int*) realloc( tl, maxl * sizeof(int) );
    newtltn = (int*) realloc( tltn, maxl * sizeof(int) );
    if( newtl==NULL || newtltn==NULL ) 
      { printf( "*** not enough memory (ExpandTL)\n" ); exit(3); }
    else 
      { tl=newtl; tltn=newtltn; }
  }
  
} /* ExpandTL */

void ExpandAtp()
{
  int *newatpp, *newatpt, *newatpw;

  if( fatp>=maxatp ) 
  {
    maxatp+=atpDELTA;
    newatpp = (int*) realloc( atpp, maxatp * sizeof(int) );
    newatpt = (int*) realloc( atpt, maxatp * sizeof(int) );
    newatpw = (int*) realloc( atpw, maxatp * sizeof(int) );
    if( newatpp==NULL || newatpt==NULL || newatpw==NULL )
      { printf( "*** not enough memory (ExpandAtp)\n" ); exit(3); }
    else 
      { atpp=newatpp; atpt=newatpt; atpw=newatpw; }
  }

} /* ExpandAtp */

void ExpandApt()
{
  int *newaptp, *newaptt, *newaptw;

  if( fapt>=maxapt ) 
  {
    maxapt+=aptDELTA;
    newaptp = (int*) realloc( aptp, maxapt * sizeof(int) );
    newaptt = (int*) realloc( aptt, maxapt * sizeof(int) );
    newaptw = (int*) realloc( aptw, maxapt * sizeof(int) );
    if( newaptp==NULL || newaptt==NULL || newaptw==NULL )
      { printf( "*** not enough memory (ExpandApt)\n" ); exit(3); }
    else 
      { aptp=newaptp; aptt=newaptt; aptw=newaptw; }
  }

} /* ExpandApt */

void ExpandAtt()
{
  int *newatt1, *newatt2;

  if( fatt>=maxatt ) 
  {
    maxatt+=attDELTA;
    newatt1 = (int*) realloc( att1, maxatt * sizeof(int) );
    newatt2 = (int*) realloc( att2, maxatt * sizeof(int) );
    if( newatt1==NULL || newatt2==NULL )
      { printf( "*** not enough memory (ExpandAtt)\n" ); exit(3); }
    else 
      { att1=newatt1; att2=newatt2; }
  }

} /* ExpandAtt */

void ReadNDR( FILE * f )
{
 int i, found, p, t, inames, len, w, mup, tt, ii;
 char *name1, *name2;

 m=0; n=0; l=0;
 while( ! feof( f ) )
 {
   ExpandNames();
      
   fgets( str, MAXSTRLEN, f ); 
   if( feof(f) ) break;
   if( str[0]=='#' ) continue; /* comment line */
   
   len=strlen(str); i=0;
   SwallowSpace( str, &i );
   if( i==len ) continue; /*empty line */
   
   switch( str[i++] )
   {
     case 'p':
     	SwallowSpace( str, &i );
	while( ! IsSpace( str,i) && i<len )i++; /* x */
	SwallowSpace( str, &i );
	while( ! IsSpace( str,i) && i<len )i++; /* y */
	SwallowSpace( str, &i );
	ExpandP();
	pn[ ++m ] = fnames;
	mu[ m ] = 0;
	GetName( &i, &fnames );
	found=0;
	for( p=1; p<m; p++ )
	  if( strcmp( names+pn[p], names+pn[m] )==0 ) { found=1; break; }
	if( ! found )
	{  
	  p=m;
	  for( t=1; t<=n; t++ )
	      if( strcmp( names+tn[t], names+pn[m] )==0 ) { found=1; break; }
	}
	if( found ) { printf( "*** duplicate name: %s\n", names+pn[m] ); exit(2); }
	
	/* marking */
	SwallowSpace( str, &i );
        mup=atoi( str+i );
        mu[ p ]=mup;	  
	break;
	
     case 't':
       	SwallowSpace( str, &i );
	while( ! IsSpace( str,i) && i<len )i++; /* xpos */
	SwallowSpace( str, &i );
	while( ! IsSpace( str,i) && i<len )i++; /* ypos */
	SwallowSpace( str, &i );
	ExpandT();
	tn[ ++n ] = fnames;
	GetName( &i, &fnames );
	found=0;
	for( p=1; p<=m; p++ )
	  if( strcmp( names+pn[p], names+tn[n] )==0 ) { found=1; break; }
	if( ! found )
	{  
	  for( t=1; t<n; t++ )
	      if( strcmp( names+tn[t], names+tn[n] )==0 ) { found=1; break; }
	}
	if( found ) { printf( "*** duplicate name: %s\n", names+tn[n] ); exit(2); }
	// tuta1
	SwallowSpace( str, &i );
	while( ! IsSpace( str,i) && i<len )i++; /* anchor */
	SwallowSpace( str, &i );
	while( ! IsSpace( str,i) && i<len )i++; /* eft */
	SwallowSpace( str, &i );
	while( ! IsSpace( str,i) && i<len )i++; /* lft */
	SwallowSpace( str, &i );
	while( ! IsSpace( str,i) && i<len )i++; /* anchor */
	SwallowSpace( str, &i );
	ii=fnames;
	GetName( &i, &fnames );
	if(memcmp(HSN_prefix,names+ii,HSN_prefix_length)==0)
	{
//printf("%d %s\n",n,names+ii);
	   ExpandTL();
           tl[ ++l ]=ii+HSN_prefix_length; tltn[ l ]=n;
           names[fnames-3]='\0';
           fnames-=2;         
	}
	break;
      
     case 'e':
	SwallowSpace( str, &i );
	inames=fnames;
	name1=names+inames;
	GetName( &i, &inames );
	SwallowSpace( str, &i );
	if( isdigit(str[i])) while( ! IsSpace( str,i) && i<len )i++; /* rad */
	SwallowSpace( str, &i );
	if( isdigit(str[i])) while( ! IsSpace( str,i) && i<len )i++; /* ang */
	SwallowSpace( str, &i );
	name2=names+inames;
	GetName( &i, &inames );
	/* start from end */
	i=strlen( str )-1;
	while( IsSpace( str,i) && i>0 )i--; 
	while( ! IsSpace( str,i) && i>0 )i--; /* anchor */
	while( IsSpace( str,i) && i>0 )i--;
	while( ! IsSpace( str,i) && i>0 )i--; /* weight */
	w=atoi( str+i+1 ); /* multiplicity */
		
	/* recognize arc */
	
	if( fapt>=maxapt || fatp>=maxatp ) { ExpandAtp(); ExpandApt(); }
	
	found=0;
	for( p=1; p<=m; p++ )
	  if( strcmp( names+pn[p], name1 )==0 ) { ExpandApt(); aptp[fapt]=p; found=1; break; }
	if( found )
	{
	  found=0;
	  for( t=1; t<=n; t++ )
	    if( strcmp( names+tn[t], name2 )==0 ) { aptw[fapt]=w; aptt[fapt++]=t; found=1; break; }
	  if( ! found ) { printf( "*** unknown arc: %s -> %s\n", name1, name2 ); exit(2); }
	}
	else
	{	
	  found=0;
	  for( t=1; t<=n; t++ )
	    if( strcmp( names+tn[t], name1 )==0 ) { found=1; break; }
	  if( ! found ) { printf( "*** unknown arc: %s -> %s\n", name1, name2 ); exit(2); }
	  found=0;
	  for( p=1; p<=m; p++ )
	    if( strcmp( names+pn[p], name2 )==0 ) { ExpandAtp(); atpt[fatp]=t;
	                                            atpw[fatp]=w; atpp[fatp++]=p; found=1; break; }
	  if( ! found )
	  {
	    found=0;
	    for( tt=1; tt<=n; tt++ ) // tuta
	      if( strcmp( names+tn[tt], name2 )==0 ) { // printf("tt: %d->%d\n",t,tt);
	                                               ExpandAtt(); att1[fatt]=t;
	                                               att2[fatt++]=tt; found=1; break; }
	    if( ! found ) { printf( "*** unknown arc: %s -> %s\n", name1, name2 ); exit(2); }
	  }
	}
	break;
     
     case 'h':
       SwallowSpace( str, &i );
       netname = fnames;
       GetName( &i, &fnames );
       break;
	
   } /* switch */    
 } /* while */
}/* ReadNDR */

void WriteNMP( FILE * f )
{
  int p; 
  
  for( p=1; p<=m; p++ )
  {
    fprintf( f, "; %d %s\n", p, names + pn[p]  );
  }

}/* WriteNMP */

void WriteNMT( FILE * f )
{
  int t; 
  
  for( t=1; t<=n; t++ )
  {
    fprintf( f, "; %d %s\n", t, names + tn[t] );
  }

}/* WriteNMT */

struct pl_sub {
  char * cptype;
  char * cphnam;
  char * cplnum;
};

void ProcessHSNlabels( FILE * f )
{
  int i,j,t,isubn,cptype,cphname,cplnum,pst,hp,lp,v1,v2,p,found;
  struct l2 * qq=NULL;
  struct l2 * el2;
  struct pl_sub * e;
  
  for(j=1;j<=l;j++)
  {
    // tuta3
    t=tltn[ j ];
    strcpy(str,names+tl[ j ]);
//fprintf( f, "%s\n", str );
    i=0;
    SwallowSpace(str,&i);
    isubn=fnames;
    GetName(&i,&fnames);   
//printf("substitute transition %d (%s) by subnet %s\n", t, names+tl[ j ], names+isubn );
    pst=0;
    // reset queue
    while(str[i]!='\0')
    {
      SwallowSpace(str,&i);
      cptype=fnames;
      GetName(&i,&fnames);
      SwallowSpace(str,&i);
      cphname=fnames;
      GetName(&i,&fnames);
      SwallowSpace(str,&i);
      cplnum=fnames;
      GetName(&i,&fnames);
//printf("place type %s name %s merged with %s\n",names+cptype,names+cphname,names+cplnum);
      e=malloc(sizeof(struct pl_sub));
      if(e==NULL) {printf("karaul e!\n"); exit(13);}
      e->cptype=names+cptype;
      e->cphnam=names+cphname;
      e->cplnum=names+cplnum;
      el2=malloc(sizeof(struct l2));
      if(el2==NULL) {printf("karaul el2!\n"); exit(13);}
      el2->content=(void *)e;
      in_l2_tail(&qq,el2 );
      pst++;
      // add to queue and increment counter
    }
    fprintf( f, "; HSN substitution transition: t nmp subnet\n"); 
    fprintf(f,"%d %d %s\n",t,pst,names+isubn); 
    fprintf( f, "; HSN place mapping: hp lp\n"); 
    while((el2=from_l2_head( &qq ))!=NULL) 
    {
      e=(struct pl_sub *)el2->content;
      found=0;
//printf("mapping: %s\n",e->cphnam);
	for( p=1; p<=m; p++ )
	  if( strcmp( names+pn[p], e->cphnam )==0 ) { found=1; hp=p; break; }
	  //else printf("other name: %s\n",names+pn[p]);
      if(found==0)
      {
        printf("*** error: invalid HSN label place name %s\n",e->cphnam);
        exit(3);
      }
      lp=atoi(e->cplnum);
      if(strcmp(e->cptype,"i")==0)
      {
        v1=hp; v2=lp;
      }
      else if(strcmp(e->cptype,"o")==0)
      {
        v1=hp; v2=-lp;
      }
      else if(strcmp(e->cptype,"s")==0)
      {
        v1=-hp; v2=lp;
      }
      else if(strcmp(e->cptype,"f")==0)
      {
        v1=-hp; v2=-lp;
      } 
      else
      {
        printf("*** error: invalid HSN label place type %s\n",e->cptype);
        exit(3);
      }
      fprintf(f,"%d %d\n",v1,v2);
      free(e); 
      free(el2);
    }
    
    // print ht np <subnet>
    // print queue hp  lp
    // free queue
    
  }
}/* ProcessHSNlabels */

void WriteLSN( FILE * f )
{
  int i, p, nnmu=0; 
  
  for( p=1; p<=m; p++ )
    if(mu[p]>0) nnmu++;
  
  fprintf( f, "; LSN obtained from NDR\n");
  fprintf( f, "; m n narcs nnmu, nst\n");
  fprintf( f, "%d %d %d %d %d\n", m, n, fapt+fatp+fatt, nnmu, l );
  
  fprintf( f, "; p->t: p t w\n");
  for( i=0; i<fapt; i++ )
    fprintf( f, "%d %d %d\n", aptp[i], aptt[i], (aptw[i]>0)?aptw[i]:-1 );
  
  fprintf( f, "; t->p: -p t w\n");  
  for( i=0; i<fatp; i++ )
    fprintf( f, "%d %d %d\n", -atpp[i], atpt[i], atpw[i] );
  
  fprintf( f, "; t->t: -t1 -t2 0\n");    
  for( i=0; i<fatt; i++ )
    fprintf( f, "%d %d %d\n", -att1[i], -att2[i], 0 );
    
  fprintf( f, "; mu(p):\n");
  for( p=1; p<=m; p++ )
    if(mu[p]>0) fprintf( f, "%d %d\n", p, mu[p] );
      
  if(l>0) 
  {
     ProcessHSNlabels( f );
  }
  
  fprintf( f, "; Table of places\n; no name\n");
  WriteNMP( f );
  
  fprintf( f, "; Table of transitions\n; no name\n");
  WriteNMT( f );
  
  fprintf( f, "; end of LSN\n");

}/* WriteLSN */

void WriteNMP_matr_h( FILE * f )
{
  int p; 
  
  for( p=1; p<=m; p++ )
  {
    fprintf( f, "// %d\t%s\n", p-1, names + pn[p]  );
  }

}/* WriteNMP_matr_h */

void WriteNMT_matr_h( FILE * f )
{
  int t; 
  
  for( t=1; t<=n; t++ )
  {
    fprintf( f, "// %d\t%s\n", t-1, names + tn[t] );
  }

}/* WriteNMT_mart_h */

#define MATRIX_SIZE(d1,d2,t) ((d1)*(d2)*(sizeof(t)))
#define MOFF(i,j,d1,d2) ((d2)*(i)+(j))
#define MELT(x,i,j,d1,d2) (*((x)+MOFF(i,j,d1,d2)))

void prnMartC(FILE * f,int *x,int m,int n)
{
  int i,j;
  fprintf( f, "%s","{\n");
  for(i=0;i<m;i++)
  {
  	fprintf( f, "%c",'{');
  	for(j=0;j<n;j++)
  	{
  		fprintf( f, "%d",MELT(x,i,j,m,n));
  		fprintf( f, "%c",(j<n-1)?',':'}');
	}
	fprintf( f, "%s",(i<m-1)?",\n":"\n");
  }
   fprintf( f, "%s","};\n");
}

// Generate a matrix of priority arc chains. @ 2023 Qing Zhang: zhangq9919@163.com, Dmitry Zaitsev
void priority_chain(int *R,int n)
{
 int x, y, z;

    // lower triangle
    for (x = 0; x < n; x++) {
        for (y = 0; y < x; y++) {
            if (MELT(R,x,y,n,n) != 0) {
                for (z = 0; z < n; z++) {
                    if (MELT(R,y,z,n,n) != 0) {
                        MELT(R,x,z,n,n) = 1;
                    }
                }
            }
        }
    }

    // upper triangle
    for (x = 0; x < n; x++) {
        for (y = x + 1; y < n; y++) {
            if (MELT(R,x,y,n,n) != 0) {
                for (z = 0; z < n; z++) {
                    if (MELT(R,y,z,n,n) != 0) {
                        MELT(R,x,z,n,n)= 1;
                    }
                }
            }
        }
    }
}
// END: Generate a matrix of priority arc chains. @ 2023 Qing Zhang: zhangq9919@163.com, Dmitry Zaitsev


void WriteSN_matr_h( FILE * f )
{
  int i,p; 
  int * x;
  
  x=malloc(MATRIX_SIZE(m,n,int));
   
  fprintf( f, "// SN obtained from NDR\n");
  fprintf( f, "#define m %d\n#define n %d\n", m, n);
  memset(x,0,MATRIX_SIZE(m,n,int));
  fprintf( f, "// incoming arcs of transitions\nstatic int b[%d][%d]=\n",m,n);
  for( i=0; i<fapt; i++ )
    MELT(x,(aptp[i]-1),(aptt[i]-1),m,n)=(aptw[i]>0)?aptw[i]:-1;
    //fprintf( f, "%d %d %d\n", aptp[i], aptt[i], (aptw[i]>0)?aptw[i]:-1 );
  prnMartC(f,x,m,n);
 
  memset(x,0,MATRIX_SIZE(m,n,int));
  fprintf( f, "// outgoing arcs of transitions\nstaticint d[%d][%d]=\n",m,n);  
  for( i=0; i<fatp; i++ )
    MELT(x,(atpp[i]-1),(atpt[i]-1),m,n)=atpw[i];
    //fprintf( f, "%d %d %d\n", -atpp[i], atpt[i], atpw[i] );
  prnMartC(f,x,m,n);
    
  /*fprintf( f, "; t->t: -t1 -t2 0\n");    
  for( i=0; i<fatt; i++ )
  fprintf( f, "%d %d %d\n", -att1[i], -att2[i], 0 );*/
    
  free(x);
  x=malloc(MATRIX_SIZE(n,n,int));
  memset(x,0,MATRIX_SIZE(n,n,int));
  fprintf( f, "// priority arcs connecting transitions, transitive closure\nstaticint r[%d][%d]=\n",n,n);   
  for( i=0; i<fatt; i++ )
    MELT(x,(att1[i]-1),(att2[i]-1),n,n)=1;
    //fprintf( f, "%d %d %d\n", -att1[i], -att2[i], 0 );
  priority_chain(x,n);  
  prnMartC(f,x,n,n);

  free(x);
  fprintf( f, "// initial marking\nstaticint mu[%d]={",m);
  for( p=1; p<=m; p++ )
  {
  	fprintf( f, "%d", mu[p] );
  	fprintf( f, "%c",(p<m)?',':'}');
  }
  fprintf( f, ";\n");
      
  /*if(l>0) 
  {
     ProcessHSNlabels( f );
  }*/
  
  fprintf( f, "// Table of places\n// no\tname\n");
  WriteNMP_matr_h( f );
  
  fprintf( f, "// Table of transitions\n// no\tname\n");
  WriteNMT_matr_h( f );
  
  fprintf( f, "// end of SN\n");

}/* WriteSN_matr_h */


int NDRtoLSN( char * NetFileName, char * LSNFileName, int write_name_tables, int matr )
{
 char nFileName[ FILENAMELEN+1 ];
 FILE * NetFile, * LSNFile, * nFile, * OutFile;
 int format;
 int z;
   
 /* open files */
 if( strcmp( NetFileName, "-" )==0 ) NetFile = stdin;
   else NetFile = fopen( NetFileName, "r" );
 if( NetFile == NULL ) {printf( "*** error open file %s\n", NetFileName );exit(2);}
 if( strcmp( LSNFileName, "-" )==0 ) LSNFile = stdout;
   else LSNFile = fopen( LSNFileName, "w" );
 if( LSNFile == NULL ) {printf( "*** error open file %s\n", LSNFileName );exit(2);}
   
 /* init net size  */
 maxn=nINIT;
 maxm=mINIT; 
 maxl=lINIT;
 maxnames=namesINIT;
 maxatp=atpINIT;
 maxapt=aptINIT;
 maxatp=attINIT;

 /* allocate arrays */
 tn = (int*) calloc( maxn, sizeof(int) ); n=0;
 tl = (int*) calloc( maxl, sizeof(int) ); l=0;
 tltn = (int*) calloc( maxl, sizeof(int) ); 
 
 pn = (int*) calloc( maxm, sizeof(int) ); m=0;
 mu = (int*) calloc( maxm, sizeof(int) );
 
 names = (char*) calloc( maxnames, sizeof(char) ); fnames=0;
 aptp = (int*) calloc( maxapt, sizeof(int) );
 aptw = (int*) calloc( maxapt, sizeof(int) );
 aptt = (int*) calloc( maxapt, sizeof(int) ); fapt=0;
 atpp = (int*) calloc( maxatp, sizeof(int) );
 atpw = (int*) calloc( maxatp, sizeof(int) );
 atpt = (int*) calloc( maxatp, sizeof(int) ); fatp=0;
 att1 = (int*) calloc( maxatt, sizeof(int) );
 att2 = (int*) calloc( maxatt, sizeof(int) ); fatt=0;

 if( tn==NULL || tl==NULL || tltn==NULL ||
     pn==NULL || 
     mu==NULL ||
     names==NULL ||
     aptp==NULL || aptt==NULL || aptw==NULL ||
     atpp==NULL || atpt==NULL || atpw==NULL ||
     att1==NULL || att2==NULL )
   { printf( "*** not enough memory for net\n" ); return(3); }  
   
 
 ReadNDR( NetFile ); 
 
 if( NetFile != stdin ) fclose( NetFile );

 if( matr) WriteSN_matr_h( LSNFile ); else WriteLSN( LSNFile );
 if( LSNFile != stdout )fclose( LSNFile );
 
 if(write_name_tables)
 {
   sprintf( nFileName, "%s.nmp", LSNFileName );
   nFile = fopen( nFileName, "w" );
   if( nFile == NULL ) {printf( "*** error open file %s\n", nFileName );exit(2);}
   WriteNMP( nFile );
   fclose( nFile );
 
   sprintf( nFileName, "%s.nmt", LSNFileName );
   nFile = fopen( nFileName, "w" );
   if( nFile == NULL ) {printf( "*** error open file %s\n", nFileName );exit(2);}
   WriteNMT( nFile );
   fclose( nFile );
 }

 free( tn ); free(atpp); free(atpw); free(atpt);
 free( tl ); free( tltn );
 
 free( pn ); free(aptp); free(aptw); free(aptt);
 
 free(att1); free(att2);
  
 free( names );
 
 return(0);
 
}/* NDRtoLSN */

#ifdef __MAIN__
static char Help[] =
"NDRtoSN - version 2.0.2\n\n"
"action: converts .ndr file to either .lsn/.hsn or C language header .h\n"
"file formats: .ndr, .net (www.laas.fr/tina), .lsn/.hsn, C header .h\n"
"usage:   NDRtoSN [-h]\n"
"                 [-l/-c]\n"
"                 ndr_file lsn_hsn_file/c_header_file\n"
"FLAGS            WHAT                                          DEFAULT\n"
"-h               print help (this text)\n"
"-l               output as .lsn/.hsn                           -l\n"
"-c               output as C header\n" 
"ndr_file         Sleptsov/Petri net in .ndr format\n"
"lsn_or_hsn_file  Sleptsov/Petri net in .lsn or .hsn format\n"
"c_header_file    Sleptsov/Petri as C language header\n\n"
"@ 2024 Dmitry Zaitsev, daze@acm.org\n";
int main( int argc, char *argv[] )
{
  char * InFileName;
  char * OutFileName;
  int i, numf=0, c_headers=0;
  
    /* parse command line */
    numf=0;
    for( i=1; i<argc; i++ )
    {
      if( strcmp( argv[i], "-h" )==0 )
      {
        printf( "%s", Help ); return( 0);
      }
      
      else if( strcmp( argv[i], "-c" )==0 ) c_headers=1;
      else if( strcmp( argv[i], "-l" )==0 ) c_headers=0;
      
      else if( numf==0 ) { InFileName=argv[i]; numf++; }
      else if( numf==1 ) { OutFileName=argv[i]; numf++; }
      else
        { printf( "*** unknown option: %s\n", argv[i] ); return(4); }
    } /* for */
  
    if( numf==0 ) InFileName = "-";
    if( numf<=1 ) OutFileName = "-";
   
    NDRtoLSN( InFileName, OutFileName, 0, c_headers);
   
  return(0);
  
} /* main */

#endif


// @ Dmitry Zaitsev 2024 daze@acm.org

